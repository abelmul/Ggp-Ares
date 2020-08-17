#ifndef PROVER_HH
#define PROVER_HH

#include "reasoner/substitution.hh"
#include "utils/gdl/gdl.hh"
#include "utils/game/match.hh"
#include "utils/utils/exceptions.hh"
#include "utils/gdl/gdlParser/gdlParser.hh"
#include "utils/memory/memCache.hh"
#include "reasoner/cache.hh"
#include "reasoner/unifier.hh"
#include "utils/utils/cfg.hh"
#include "utils/threading/threading.hh"

namespace ares
{ 
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
         * Carry out backward chaining. Search the sld-tree for a successful refutation.
         * Using kb + context as a combined knowledgebase.
         * @param query.context : The current state determined by true and does
         * @param query.cb is called with an answer each time a successful refutation is derived.
         */
        void compute(Query& query);  
        ~Prover(){
            if(proverPool) delete proverPool;
        }
    private:
        /**
         * Extension of compute(Query& query), needed for recursion.
         */ 
        void compute(Query query,Cache* cache);
        /**
         *
         */
        void compute(cnst_lit_sptr lit,const State* context, Cache* cache, CallBack* cb);
        /**
         * Iterate over all of the new solutions in `it` and resolve against lit
         * @param it a list of new solutions for lit
         */
        void lookup(AnsIterator& it, Query& query,cnst_lit_sptr& lit,Cache* cache);
        /**
         * Carry out a single (normal) SLD-Resolution Step
         * Where the selected literal in goal is positive.
         */
        Clause* resolve(const cnst_lit_sptr& lit, const Clause& c,SuffixRenamer& vr);
        /**
         * Carry out a single SLDNF-Resolution Step
         * Where the selected literal in goal is negative.
         */
        void computeNegation(Query& query,Cache* cache);
        /**
         * The distinct relation.
         */
        void computeDistinct(Query& query,Cache* cache);
        inline bool contextual(const State* context, const cnst_lit_sptr& g) const{
            return context and (  g->get_name() == Namer::DOES  or  g->get_name() == Namer::TRUE) ;
        }


        static Prover* _prover;
        static SpinLock slock;
        const KnowledgeBase* kb;
        Substitution* renamer;
        ThreadPool* proverPool = nullptr;     //Used for the initial query, and subequent searches.   

        friend class ClauseCB;
    }; 
} // namespace ares
#endif