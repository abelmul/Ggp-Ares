#ifndef PROVER_HH
#define PROVER_HH

#include "reasoner/substitution.hh"
#include "utils/gdl/clause.hh"
#include <condition_variable>
#include "utils/game/match.hh"
#include "utils/utils/exceptions.hh"
#include "utils/gdl/gdlParser/gdlParser.hh"
#include "utils/memory/memCache.hh"
#include "reasoner/unifier.hh"
#include "utils/utils/cfg.hh"
#include "utils/threading/threading.hh"

namespace ares
{ 
    /**
     * TODO: Look into caching
     */
    typedef std::function<void(std::reference_wrapper<const Substitution>)> CallBack;

    struct Query{
        typedef std::unique_ptr<Clause> unique_clause;
        Query(std::unique_ptr<Clause> _g,const  State* _c ,const CallBack& _cb, const bool _one,bool& d)
        :goal(std::move(_g)),context(_c), cb(_cb), oneAns(_one),done(d)
        {
        }
        Query(const Query& q)
        :goal( std::move(*(unique_clause*)&q.goal)),context(q.context),cb(q.cb),pool(q.pool),renamer(q.renamer),oneAns(q.oneAns),done(q.done)
        {

        }
        // Query(Query&& q)
        // :goal( std::move(q.goal)),context(q.context),cb(q.cb),pool(q.pool),renamer(q.renamer),oneAns(q.oneAns),done(q.done)
        // {
        // }
        std::unique_ptr<Clause> goal;
        const State* const context;
        const CallBack& cb;                    //This is used to return the computed answer(s) to the caller in a multi-threaded enviroment.
        ThreadPool* pool = nullptr;
        SuffixRenamer renamer;
        const bool oneAns;
        bool& done;
    };

    class Prover
    {
    private:
        Prover(const KnowledgeBase* _kb)
        :kb(_kb)
        {
            if( cfg.proverThreads > 0 ) proverPool = new ThreadPool(new LoadBalancerRR(cfg.proverThreads));
        };
    public:
        static Prover* getProver(const KnowledgeBase* _kb){
            slock.lock();
            if(not  _prover) _prover = new Prover(_kb);
            slock.unlock();
            return _prover;     //singleton
        }
        /**
         * @param state : The current state determined by true and does
         * Carry out backward chaining. Search the sld-tree for a successful refutation.
         * Using kb + state as a combined knowledgebase.
         * @param query.one : if one == true, the method immediately returns if a single refutation is derived,
         * otherwise all such successful refutations are derived.
         * @param query.cb is called each time a successful refutation is derived.
         */
        void prove(Query query);  
        void setKB(const KnowledgeBase* _kb){kb = _kb;}
        ~Prover(){
            if(proverPool) delete proverPool;
        }
    private:
        /**
         * Extension of prove(Query& query), needed for recursion.
         */ 
        void _prove(Query query);
        /**
         * Carry out a single (normal) SLD-Resolution Step
         * Where the selected literal in goal is positive.
         */
        Clause* resolve(const Clause& goal, const Clause& c,SuffixRenamer& vr);
        /**
         * Carry out a single SLDNF-Resolution Step
         * Where the selected literal in goal is negative.
         */
        bool handleNegation(Clause& goal, const State* context,SuffixRenamer& renamer);
        /**
         * The distinct relation.
         */
        bool proveDistinct(Clause& goal);
        inline bool contextual(const Query& q, const cnst_lit_sptr& g) const{
            return q.context and (  g->get_name() == Namer::DOES  or  g->get_name() == Namer::TRUE) ;
        }


        static Prover* _prover;
        static SpinLock slock;
        const KnowledgeBase* kb;
        Substitution* renamer;
        ThreadPool* proverPool = nullptr;     //Used for the initial query, and subequent searches.
        bool done;
        std::mutex mDone;
        //Some thread will notify through this cv when either one ans is computed or sld tree is exhaustively searched.
        std::condition_variable cvDone;         
    }; 
} // namespace ares
#endif