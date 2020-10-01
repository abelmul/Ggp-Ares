#ifndef MONTE_SEQ_HH
#define MONTE_SEQ_HH
#include <algorithm>
#include <future>
#include <condition_variable>
#include <chrono>
#include <mutex>
#include "strategy.hh"
#include "utils/utils/iterators.hh"
#include "utils/threading/threading.hh"
#include <limits>


namespace ares{
    class MontecarloSeq : public Strategy, public RegistrarBase<MontecarloSeq>
    {

    public:
        // typedef std::numeric_limits<float> INFINITY
        struct ISelectionPolicy;
        struct ISimPolicy;
        
        struct Node{
            Node(const State* s,const Action* a)
            :n(0),value(0),parent(nullptr),state(s),actions(s,reasoner),action(a)
            {
            }
            /*methods*/
            void add(Node *child){ std::lock_guard<std::mutex> lk(lock); children.push_back(child);}
            void erase();                               //Delete the subtree rooted at this node
            /*data*/
            uint n;                                     //#Simulations through this node
            float value;                                 //Total value
            Node* parent;
            std::unique_ptr<const State> state;
            ActionIterator actions;                     //The available actions from this nodes state
            std::vector<Node*> children;    
            ucAction action;                            //The action that was taken from the parents state
            std::mutex lock;
        };

    /**
     * Methods
     */
    public:
        MontecarloSeq()
        :tree(*this),timer(*this),selPolicy(nullptr),simPolicy(nullptr)
        {}
        virtual void init(Reasoner*);
        /**
         * Dump the constructed tree
         */
        virtual void dump(std::string str="{}");
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
            stoped = true;
            timer.cancel();
            std::lock_guard<std::mutex> lk(lock);
            tree.reset();
        }

        virtual std::string name(){return "MontecarloSeq";}
        static MontecarloSeq* create(){ static MontecarloSeq monte; return &monte;}

        inline void setPolicies(ISelectionPolicy* s,ISimPolicy* sim_){
            if(selPolicy) delete selPolicy;
            if(simPolicy) delete simPolicy;
            selPolicy=s; simPolicy = sim_;
        }
        
        /**
         * @param def the default value to return for unexplored nodes.
         */
        inline static float uct(const Node& nd,ushort c,float def=0) 
        { return  nd.n == 0 ? def :(nd.value/nd.n) +( c * sqrt( (2*std::log(nd.parent->n))/ nd.n)) ; }
        /**
         * Choose the best child according to uct.
         * @param n the node whose children (>0) are to be compared
         * @param def the default value to use for unexplored nodes.
         */
        inline static Node* bestChild(Node& n,ushort c,float def=0){
            std::unique_lock<std::mutex> lk;
            if( !n.parent )lk= std::unique_lock<std::mutex>( n.lock);  //There is only conccurecy at the root node
            return *std::max_element(n.children.begin(),n.children.end(),
                                    [&](auto& n, auto& m) {return uct(*n,c,def) < uct(*m,c,def);});
        }
        virtual ~MontecarloSeq();

    private:
        /**
         * update v's value and Propagate val through ancestors of v
         */
        inline void update(Node* v,ushort val){
            auto update_ =[&](ushort& val){
                v->n++;
                v->value += val;
                v = v->parent;
            };
            while (v )
            {
                if( v->parent == tree.root){//timer might go off and try to select best child of root
                    std::lock_guard<std::mutex> lk(tree.root->lock);
                    update_(val);
                    continue;
                }
                update_(val);
            }
        }

    /**
     * inner structs
     */
    private:    
        //The tree
        struct Tree{
            Tree(MontecarloSeq& m):mc(m),root(nullptr),origRoot(nullptr){}
            void select(const Action* action);
            void reset(MontecarloSeq::Node* root_=nullptr);
            ~Tree(){ reset(); }
            /*Data*/
            MontecarloSeq& mc;
            std::mutex lock;
            Node* root;    // Keep track of current state. This dynamically changes on every step.
            Node* origRoot;  //For debugging and visualization purposes don't delete the original tree.
        };
        /**
         * The timer, to assert that replies are made with the assigned time
         */
        class Timer{
            public: 
                Timer(MontecarloSeq& mc):mct(mc),seq(0),done(nullptr){}
                std::future<const Term*> reset(std::atomic_bool& done_,const Match& match);
                /**
                 * Cancel any scheduled timers;
                 */ 
                void cancel();
                /**
                 * Cancel the current timer if stored done matches given done.
                 */
                void cancel(std::atomic_bool* done_);
            private: 
                MontecarloSeq& mct;
                std::atomic_uint seq;
                std::atomic_bool* done;
                std::mutex lock;
                std::condition_variable cv;
        };
    public :
        struct ISelectionPolicy
        { 
            virtual Node* operator()(Node*,const std::atomic_bool&)const=0;
            virtual ~ISelectionPolicy(){} 
        };
        struct ISimPolicy
        { 
            virtual float operator()(Node*,ushort,std::atomic_bool&)const=0; 
            virtual ~ISimPolicy(){}
        };
    /**
     * Data
     */
    private:
        Tree tree;
        Timer timer;
        std::mutex lock;
        ISelectionPolicy* selPolicy; ISimPolicy* simPolicy;
        std::atomic_bool stoped;
        
    public:
    friend class Timer;
    friend class MonteTester;
    };//End Class MontecarloSeq
}//namespace ares
#endif