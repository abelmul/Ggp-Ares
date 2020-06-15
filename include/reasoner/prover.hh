#ifndef PROVER_HH
#define PROVER_HH

#include "reasoner/substitution.hh"
#include "utils/gdl/clause.hh"
#include <condition_variable>
#include "utils/game/match.hh"
#include "utils/threading/threadPool.hh"
#include "utils/gdl/gdlParser/exceptions.hh"
#include "utils/gdl/gdlParser/expressionPool.hh"
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
        Query(Clause* _g,const  State* _c , const CallBack<T>* _cb, const bool _one,bool& d)
        :goal(_g),context(_c), cb(_cb), oneAns(_one),done(d)
        {
        }
        
        Clause* const goal;
        const State* const context;
        const CallBack<T>* const cb;
        const bool oneAns;
        ThreadPool* pool;
        bool& done;
    };
    /**
     * The resulting resolvent,if any, in a single sldnf resolution step.
     */
    struct Resolvent        
    {
        Clause* gn;
        bool ok;
    };
    template <class T>
    class Prover
    {
    private:
        Prover(KnowledgeBase* _kb, ushort proverThreads, ushort negThreads):kb(_kb){
            proverPool = new ThreadPool(proverThreads);
            negationPool = new ThreadPool(negThreads);
        };

    public:
        static Prover* getProver(KnowledgeBase* _kb, ushort proverThread, ushort negThreads){
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
        bool handleNegation(Query<T>& query);
        /**
         * The distinct relation.
         */
        bool proveDistinct(Clause& goal);

        bool distinct(Term& s, Term& t){
            if( not (s.isGround() and t.isGround()) )
                throw DistinctNotGround("Distinct called with : (distinct " + s.toString() + " " + t.toString() +" )");
            
            return not ( s == t);
        }

        bool contextual(const Query<T>& q, const Literal& g) const{
            return (( strcasecmp( g.getName(), "does") == 0 ) or (strcasecmp( g.getName(), "true") == 0) \
                    and q.context);
        }

        static Prover* _prover;
        static SpinLock slock;
        KnowledgeBase* kb;

        ThreadPool* proverPool;     //Used for the initial query, and subequent searches.
        ThreadPool* negationPool;   //Used when trying to prove a negative goal(literal).

        bool done;
        std::mutex mDone;
        //Some thread will notify through this cv when either one ans is computed or sld tree is exhaustively searched.
        std::condition_variable cvDone;         
    }; 
} // namespace Ares

#endif