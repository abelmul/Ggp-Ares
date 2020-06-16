#ifndef PROVER_HH
#define PROVER_HH

#include "reasoner/substitution.hh"
#include "utils/gdl/clause.hh"
#include <condition_variable>
#include "utils/game/match.hh"
#include "utils/threading/threadPool.hh"
#include "utils/gdl/gdlParser/exceptions.hh"
#include "utils/gdl/gdlParser/expressionPool.hh"
#include "reasoner/unifier.hh"
namespace Ares
{ 
    /**
     * TODO: Look into caching
     */

    /**
     * This is used to return the computed answer(s) to the caller in a multi-threaded
     * enviroment.
     */
    template <class T> 
    struct CallBack
    {
        CallBack(T _cb):cb(_cb){}
        void operator()(const Substitution& computedAns){
            std::lock_guard<std::mutex> lk(m);
            cb(computedAns);
        };
        T cb;
        std::mutex m;
    };

    template <class T> 
    struct Query{
        Query(Clause* _g,const  State* _c ,CallBack<T>& _cb, const bool _one,bool& d)
        :goal(_g),context(_c), cb(_cb), oneAns(_one),done(d)
        {
        }
        
        void deleteGoal(){ if (deletable) delete goal;}

        Clause* const goal;
        const State* const context;
        CallBack<T>& cb;
        const bool oneAns;
        ThreadPool* pool = nullptr;
        bool& done;
        bool deletable = true;      //Just to keep track of intermediate clauses arising from sld resolution step.
                                    //Only the original is non-deletable
    };
    /**
     * The resulting resolvent,if any, in a single sldnf resolution step.
     */
    struct Resolvent        
    {
        Clause* gn;
        bool ok;
    };
    
    class Prover
    {
    private:
        Prover(KnowledgeBase* _kb,ushort proverThreads, ushort negThreads)
        :kb(_kb),proverPool(new ThreadPool(proverThreads)),negationPool(new ThreadPool(negThreads))
        {
        };

    public:
        static Prover* getProver(KnowledgeBase* _kb,ushort proverThread, ushort negThreads){
            slock.lock();
            if(not  _prover) _prover = new Prover(_kb,proverThread, negThreads);
            slock.unlock();
            return _prover;     //singleton
        }
        /**
         * Carry out backward chaining. Search the sld-tree for a successful refutation.
         * Using kb + @param state as a combined knowledgebase.
         * if one == true, the method immediately returns if a single refutation is derived,
         * otherwise all such successful refutations are derived.
         * cb is called each time a successful refutation is derived.
         */
        template <class T>
        void prove(Query<T>& query);  
        void setKB(KnowledgeBase* _kb){kb = _kb;}
        
        ~Prover(){
            delete proverPool;
            delete negationPool;
        }

    private:
        /**
         * Extension of prove(Query<T>& query), needed for recursion.
         */ 
        template <class T>
        void _prove(Query<T> query);
        /**
         * Carry out a single (normal) SLD-Resolution Step
         * Where the selected literal in goal is positive.
         */
        Resolvent resolve(const Clause& goal, const Clause& c);
        /**
         * Carry out a single SLDNF-Resolution Step
         * Where the selected literal in goal is negative.
         */
        bool handleNegation(Clause& goal, const State* context);
        /**
         * The distinct relation.
         */
        bool proveDistinct(Clause& goal);

        bool distinct(Term& s, Term& t){
            if( not (s.isGround() and t.isGround()) )
                throw DistinctNotGround("Distinct called with : (distinct " + s.toString() + " " + t.toString() +" )");
            
            return not ( s == t);
        }
        
        template <class T>
        bool contextual(const Query<T>& q, const Literal& g) const{
            return ( (( strcasecmp( g.getName(), "does") == 0 ) or (strcasecmp( g.getName(), "true") == 0)) \
                    and q.context);
        }

        static Prover* _prover;
        static SpinLock slock;
        KnowledgeBase* kb;
        Substitution* renamer;
        ThreadPool* proverPool;     //Used for the initial query, and subequent searches.
        ThreadPool* negationPool;   //Used when trying to prove a negative goal(literal).

        bool done;
        std::mutex mDone;
        //Some thread will notify through this cv when either one ans is computed or sld tree is exhaustively searched.
        std::condition_variable cvDone;         
    }; 


    /**
     * Implementation for template methods of Prover.
     */

    template<class T>                                   //T is the callback functiions type
    void Prover::prove(Query<T>& query){
        query.pool = proverPool;                         //This is the initial query
        proverPool->acquire();                          //Need ownership of pool
        auto f = std::bind([&](){
            this->_prove<T>(query);
        });
        
        proverPool->post<decltype(f)>(f);               //Schedule an sld tree search to prove this query.
        proverPool->wait();                             //Wait until query is answered. Release ownership.
    }

    template<class T>                                       //T is the callback functiions type
    void Prover::_prove(Query<T> query){
        if( query.done ) return;                            //one answer has been found, No need to keep on searching.

        //Recursively explore the sld tree 
        if( Clause::EMPTY_CLAUSE(*query.goal) ){
            //Successful derivation
            if( query.oneAns ){
                query.done = true;                         //We are done no need to compute more answers.
                if( query.pool ) query.pool->stop();       //No need to schedule further searches.
            }
            query.cb.operator()(*query.goal->getSubstitution());
            return;
        }

        const Literal& g = (*query.goal).front();

        if( strcasecmp( g.getName(), "distinct") == 0 ){
            bool ok = proveDistinct(*query.goal);    
            if( !ok ) return;          
            //Has Modified query by doing query.goal.popFront()
            _prove(query);       //Just prove the next goal clause   
            return;
        }

        if( not g ){
            bool ok = handleNegation(*query.goal, query.context);
            if( !ok ) return;
            //Has Modified query by doing query.goal.popFront()
            _prove(query);       //Just prove the next goal clause   
            return;
        }

        //At this point the goal is positive, try to unify it with clauses in knowledge base.
        const KnowledgeBase* kb_t = contextual(query,g)  ? query.context : this->kb;
        
        //Get the clauses g might unify with
        const std::vector<const Clause*>& clauses = *(*kb_t)[g.getName()];
        Resolvent g_n;
        for (auto &&c : clauses)
        {
            g_n = resolve(*query.goal, *c);     //Perfom a normal sld resolution step.
            if(!g_n.ok) continue;               //Didn't unify
            //query.goal[0] and head of c unify
            Query<T> q(g_n.gn, query.context, query.cb, query.oneAns, query.done);  //The next goal clause to prove
            q.pool = query.pool;
            //Pool Might not be available due to proving negations.
            auto handler = std::bind([this,q](){
                this->_prove(q);
            });
            // auto handler = std::bind(&Prover::_prove, this, q );
            if ( not q.pool ) 
                this->_prove<T>(q);

            else 
                q.pool->template post<decltype(handler)>(handler);
        }
        query.deleteGoal();     //Won't ever use goal again.
    }

} // namespace Ares

#endif