#ifndef PROVER_HH
#define PROVER_HH

#include "reasoner/substitution.hh"
#include "utils/gdl/clause.hh"
#include <condition_variable>
#include "utils/game/match.hh"
#include "utils/threading/threadPool.hh"
#include "utils/gdl/gdlParser/exceptions.hh"

namespace Ares
{ 
    /**
     * TODO: Look into caching
     */

    /**
     * This is used to return the computed answer(s) to the caller in a multi-threaded
     * enviroment.
     */
    struct CallBack;
    struct Query;
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
        Prover(KnowledgeBase* _kb, ushort proverThreads, ushort negThreads):kb(_kb){
            proverPool = new ThreadPool(proverThreads);
            negationPool = new ThreadPool(negThreads);
            waitingThreads = new ThreadPool(2);
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
        void prove(Query* query);  
        void setKB(KnowledgeBase* _kb){kb = _kb;}

        ~Prover(){
            delete proverPool;
            delete negationPool;
            delete waitingThreads;
        }

    private:
        /**
         * Extension of prove(Query* query), needed for recursion.
         */ 
        void _prove(Query* query);
        /**
         * Carry out a single (normal) SLD-Resolution Step
         * Where the selected literal in goal is positive.
         */
        Resolvent* resolve(Clause& goal, Clause& c);
        /**
         * Carry out a single SLDNF-Resolution Step
         * Where the selected literal in goal is negative.
         */
        Resolvent* handleNegation(Clause& goal);
        /**
         * The distinct relation.
         */
        bool distinct(Term& s, Term& t){
            if( not (s.isGround() and t.isGround()) )
                throw DistinctNotGround("Distinct called with : (distinct " + s.toString() + " " + t.toString() +" )");
            
            return not ( s == t);
        }
        /**
         * Will Return when either of the following condition is met.
         * 1. query->oneAns && query->waitOne() returned: 
         *      In this case on answer has been computed. and 
         *      thats all we want (query->oneAns == trues).
         * 2. pool->wait() returned:
         *      The sld-tree has been exhaustively searched.
         */
        void wait(Query* query);

        static Prover* _prover;
        static SpinLock slock;
        KnowledgeBase* kb;

        ThreadPool* proverPool;     //Used for the initial query, and subequent searches.
        ThreadPool* negationPool;   //Used when trying to prove a negative goal(literal).
        ThreadPool* waitingThreads; //To wait on some conditions.

        bool done;
        std::mutex mDone;
        //Some thread will notify through this cv when either one ans is computed or sld tree is exhaustively searched.
        std::condition_variable cvDone;         
    };  
    struct CallBack
    {
        bool operator()(Substitution* computedAns){

        }
    };

    struct Query{
        Query( Clause* _g, State* _c , CallBack* _cb, bool _one)
        :goal(_g),context(_c), cb(_cb), oneAns(_one)
        {
        }
        void waitOne(){
            std::unique_lock<std::mutex> lk(mFndOne);
            cvFndOne.wait(lk, [this](){return this->fndOne;});  //Wait until one answer has been computed.
        }

        void notifyOne(){
            {
                std::unique_lock<std::mutex> lk(mFndOne);
                fndOne = true;
            }
            cvFndOne.notify_all();      //Notify that one answer has been computed.
        }

        void done(bool _d){
            std::unique_lock<std::mutex> lk(mDone);
            _done = _d;
        }
        bool done(){return _done;}
        
        ThreadPool* pool(){ return _pool;};
        void pool(ThreadPool* p ){ _pool = p;};

        const Clause* const goal;
        const State* const context;
        const CallBack* const cb;
        const bool oneAns;

        private:
            ThreadPool* _pool;
            
            bool fndOne = false;
            std::mutex mFndOne;
            std::condition_variable cvFndOne;

            bool _done = false;
            std::mutex mDone;
    };
} // namespace Ares

#endif