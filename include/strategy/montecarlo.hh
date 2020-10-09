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
#include <limits>


namespace ares{
    class Montecarlo : public Strategy, public RegistrarBase<Montecarlo>
    {

    public:
        // typedef std::numeric_limits<float> INFINITY
        struct ISelectionPolicy;
        struct ISimPolicy;
        
        struct Node{
            Node(const State* s,const Action* a)
            :n(0),values(reasoner->roles().size()),parent(nullptr),state(s),actions(s,reasoner),action(a)
            {
            }
            /*methods*/
            void add(Node *child){
                std::lock_guard<std::mutex> lk(lock); children.push_back(child);
                if( observers ) {cv.notify_all();}
            }
            std::unique_lock<std::mutex> waitUntilChildren(){
                std::unique_lock<std::mutex> lk(lock);
                observers++;
                cv.wait(lk,[&]{ return children.size() > 0;});
                observers--;
                return lk;
            }
            void erase();                               //Delete the subtree rooted at this node
            /*data*/
            uint n;                                     //#Simulations through this node
            std::vector<float> values;                   //Total value indexed by role
            Node* parent;
            std::unique_ptr<const State> state;
            ActionIterator actions;                     //The available actions from this nodes state
            std::vector<Node*> children;    
            ucAction action;                            //The action that was taken from the parents state
            std::mutex lock;
            private:
                std::condition_variable cv;
                uint observers;
        };

    /**
     * Methods
     */
    public:
        typedef uint Ptype;
        Montecarlo()
        :tree(*this),timer(*this),selPolicy(nullptr),simPolicy(nullptr)
        {}
        virtual void init(Reasoner*,GameAnalyzer*);
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
            analyzer->stop();
            std::lock_guard<std::mutex> lk(lock);
            std::lock_guard<std::mutex> tlk(tree.lock);
            order.clear();
            tree.reset();
        }

        virtual std::string name(){return "Montecarlo";}
        static Montecarlo* create(){ static Montecarlo monte; return &monte;}

        inline void setPolicies(ISelectionPolicy* s,ISimPolicy* sim_){
            if(selPolicy) delete selPolicy;
            if(simPolicy) delete simPolicy;
            selPolicy=s; simPolicy = sim_;
        }
        
        /**
         * @param c the uct parameter c
         * @param p index of the current player within reasoner.roles()
         * @param def the default value to return for unexplored nodes.
         */
        inline static float uct(const Node& nd,ushort c,ushort p,float def) 
        { return  nd.n == 0 ? def :(nd.values[p]/nd.n) +( c * sqrt( (2*std::log(nd.parent->n))/ nd.n)) ; }
        /**
         * Choose the best child according to uct.
         * @param n the node whose children (>0) are to be compared
         * @param c the uct parameter c
         * @param role index of the current player within reasoner.roles()
         * @param def the default value to use for unexplored nodes.
         */
        inline static Node* bestChild(Node& n,ushort c,ushort role,float def){
            auto lock = n.waitUntilChildren();
            return *std::max_element(n.children.begin(),n.children.end(),
                                    [&](auto& n1, auto& m) {return uct(*n1,c,role,def) < uct(*m,c,role,def);});
        }

        inline static ushort plyr(Ptype& p,ushort indx){ 
            ushort pind = order.size() ?  order[p] : indx;
            p =  (p+1) % (order.size() ? order.size() : 1);
            return pind;
        }
        virtual ~Montecarlo();

    private:
        /**
         * update v's value and Propagate val through ancestors of v
         */
        inline void update(Node* v,std::vector<float> vals){
            while (v )
            {
                //If v==root, make sure the tree doesn't change
                //otherwise the parent of v depends on v's value for bestChild selection
                std::lock_guard<std::mutex> lk(v->parent ? v->parent->lock : tree.lock);  
                v->n++;
                for (uint i =0;i < vals.size();i++)
                    v->values[i]+=vals[i];
                
                v = v->parent;
            }
        }

    /**
     * inner structs
     */
    private:    
        //The tree
        struct Tree{
            Tree(Montecarlo& m):mc(m),root(nullptr),origRoot(nullptr){}
            void select(const Action* action);
            void reset(Montecarlo::Node* root_=nullptr);
            ~Tree(){ reset(); }
            /*Data*/
            Montecarlo& mc;
            std::mutex lock;
            Node* root;                 // Keep track of current state. This dynamically changes on every step.
            Node* origRoot;            //For debugging and visualization purposes don't delete the original tree.
        };
        /**
         * The timer, to assert that replies are made with the assigned time
         */
        class Timer{
            public: 
                Timer(Montecarlo& mc):mct(mc),seq(0),done(nullptr){}
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
                Montecarlo& mct;
                std::atomic_uint seq;
                std::atomic_bool* done;
                std::mutex lock;
                std::condition_variable cv;
        };
    public :
        struct ISelectionPolicy
        { 
            virtual Node* operator()(Node*,const std::atomic_bool&,Montecarlo::Ptype,ushort indx)const=0;
            virtual ~ISelectionPolicy(){} 
        };
        struct ISimPolicy
        { 
            virtual std::vector<float> operator()(const State* state,std::atomic_bool&)const=0; 
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
        ThreadPool* pool;
        Ptype player;
        static UniqueVector<ushort> order;

    friend class Timer;
    friend class MonteTester;
    };//End Class Montecarlo
}//namespace ares
#endif