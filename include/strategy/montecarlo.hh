#ifndef MONTE_HH
#define MONTE_HH
#include <algorithm>
#include <future>
#include <condition_variable>
#include <chrono>
#include <mutex>
#include "strategy.hh"
#include "utils/utils/iterators.hh"
#include "utils/threading/threading.hh"

namespace ares{
    class Montecarlo : public Strategy, public RegistrarBase<Montecarlo>
    {
    public:
        struct ISelectionPolicy;
        struct ISimPolicy;
        
        struct Node{
            Node(const State* s,const ActionIterator::Action* a)
            :n(0),value(0),parent(nullptr),state(s),actions(s,reasoner),action(a)
            {
            }
            /*methods*/
            void add(Node *child){ std::lock_guard<std::mutex> lk(lock); children.push_back(child);}
            void erase();                               //Delete the subtree rooted at this node
            /*data*/
            uint n;                                     //#Simulations through this node
            uint value;                                 //Total value
            Node* parent;
            std::unique_ptr<const State> state;
            ActionIterator actions;                     //The available actions from this nodes state
            std::vector<Node*> children;    
            ActionIterator::UniqueAction action;       //The action that was taken from the parents state
            std::mutex lock;
        };

    /**
     * Methods
     */
    public:
        Montecarlo()
        :timer(*this)
        {}
        virtual void init();
        /**
         * Construct the game tree for match.games.
         */
        virtual void start(const Match& match);
        /**
         * Select best move from current state.
         */
        virtual move_sptr_seq operator()(const Match& match,uint);
        /**
         * Destroy game tree.
         */
        inline virtual void reset(){
            timer.cancel();
            tree.reset();
        }

        virtual std::string name(){return "Montecarlo";}
        static Montecarlo* create(){ static Montecarlo monte; return &monte;}

        inline void setPolicies(ISelectionPolicy* s,ISimPolicy* sim_){ selPolicy=s; simPolicy = sim_;}

        inline static float uct(const Node& nd,ushort c) {  return (nd.value/nd.n) +( c * sqrt( (2*std::log(nd.parent->n))/ nd.n)) ; }
        /**
         * Choose the best child according to uct.
         * @param n the node whose children (>0) are to be compared
         */
        inline static Node* bestChild(Node& n,ushort c){
            std::lock_guard<std::mutex> lk(n.lock);
            return *std::max_element(n.children.begin(),n.children.end(),[&](auto& n, auto& m){ 
                                                                            return uct(*n,c) < uct(*m,c);});
        }
        virtual ~Montecarlo();

    private:
        /**
         * update v's value and Propagate val through ancestors of v
         */
        inline void update(Node* v,ushort val){
            while (v and v->parent )
            {
                //The parent depends on its children's  value for selecting best child.
                std::lock_guard<std::mutex> lk(v->parent->lock);
                v->n++;
                v->value += val;
                v = v->parent;
            }
        }

    /**
     * inner structs
     */
    private:    
        //The tree
        struct Tree{
            Tree():root(nullptr){}
            void select(const ActionIterator::Action* action);
            void reset(Montecarlo::Node* root_=nullptr);
            ~Tree(){ reset(); }
            /*Data*/
            std::mutex lock;
            Node* root;
        };
        /**
         * The timer, to assert that replies are made with the assigned time
         */
        class Timer{
            public: 
                Timer(Montecarlo& mc):mct(mc),seq(0),done(nullptr){}
                std::future<ares::cnst_term_sptr> reset(std::atomic_bool& done_,const Match& match);
                void cancel();
            private: 
                Montecarlo& mct;
                std::atomic_uint seq;
                std::atomic_bool* done;
                std::mutex lock;
                std::condition_variable cv;
        };
         public :
            struct ISelectionPolicy{ virtual Node* operator()(Node*,const std::atomic_bool&)const=0; };
            struct ISimPolicy{ virtual float operator()(Node*,ushort)const=0; };
    /**
     * Data
     */
    private:
        Tree tree;
        Timer timer;
        std::mutex lock;
        ISelectionPolicy* selPolicy; ISimPolicy* simPolicy;

    friend class Timer;
    };//End Class Montecarlo
}//namespace ares
#endif