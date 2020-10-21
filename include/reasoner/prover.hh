#ifndef PROVER_HH
#define PROVER_HH

#include "reasoner/cache.hh"
#include "reasoner/substitution.hh"
#include "reasoner/unifier.hh"
#include "utils/game/match.hh"
#include "utils/gdl/gdl.hh"
#include "utils/gdl/gdlParser/gdlParser.hh"
#include "utils/memory/memCache.hh"
#include "utils/threading/threading.hh"
#include "utils/utils/cfg.hh"
#include "utils/utils/exceptions.hh"

namespace ares
{
    class Prover
    {
     private:
        // ctor
        Prover(const KnowledgeBase* kb_ = nullptr)
            : kb(kb_), proverCount(0), done(false)
        {
        }

        Prover(const Prover&) = delete;
        Prover& operator=(const Prover&) = delete;
        Prover(const Prover&&) = delete;
        Prover& operator=(const Prover&&) = delete;
        /**
         * Methods
         */
     public:
        /**
         * The singleton prover.
         */
        static Prover& create(const KnowledgeBase* kb_ = nullptr)
        {
            static Prover prover(kb_);
            return prover;  // singleton
        }

        inline void reset(const KnowledgeBase* kb_)
        {
            {  // Make sure there are no threads already using kb
                std::unique_lock<std::mutex> lk(lock);
                done = true;
                cv.wait(lk, [&] { return proverCount == 0; });
                done = false;
            }
            kb = kb_;
        }
        /**
         * Carry out backward chaining. Search the sld-tree for a successful
         * refutation. Using kb + context as a combined knowledgebase.
         * @param query.context : The current state determined by true and does
         * @param query.cb is called with an answer each time a successful
         * refutation is derived.
         */
        void compute(Query& query);
        ~Prover() {}

     private:
        /**
         * Extension of compute(Query& query), needed for recursion.
         */
        void compute(Query query, const bool isLookup);
        /**
         *
         */
        void compute(const Atom* lit, Query q, CallBack* cb,
                     const bool lookup = false);
        /**
         * Iterate over all of the new solutions in `it` and resolve against lit
         * @param it a list of new solutions for lit
         */
        void lookup(AnsIterator& it, Query& query, const Atom* lit);
        /**
         * Carry out a single (normal) SLD-Resolution Step
         * Where the selected literal in goal is positive.
         */
        Clause* resolve(const Atom* lit, const Clause& c, SuffixRenamer&);

        /**
         * treat <-(or α1,...,αn),β1,..,βk as if its n clauses of the form <-
         * αi,β1,..,βk
         */
        void computeOr(Query& query);
        /**
         * Carry out a single SLDNF-Resolution Step
         * Where the selected literal in goal is negative.
         */
        void computeNegation(Query& query);
        /**
         * The distinct relation.
         */
        void computeDistinct(Query& query);
        inline bool contextual(const State* context, const Atom* g) const
        {
            return context and (g->get_name() == Namer::DOES or
                                g->get_name() == Namer::TRUE);
        }

        /**
         * Data
         */
     private:
        const KnowledgeBase* kb;

        friend class ClauseCB;
        std::mutex lock;
        std::condition_variable cv;
        uint proverCount;
        bool done;
    };
}  // namespace ares
#endif