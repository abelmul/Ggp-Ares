#include "ares.hh"
#include "reasoner/prover.hh"
namespace ares
{
    std::atomic<int> Query::nextId = 0;
    AnsIterator Cache::NOT_CACHED(nullptr, -1, nullptr);

    void Prover::compute(Query& query)
    {
        {
            std::lock_guard<std::mutex> lk(lock);
            if (done)
                return;
            proverCount++;
        }
        Cache* cache = query.cache;
        auto* proverPool = ThreadPoolFactroy::get(1);
        // First stage
        proverPool->post([&]() { compute(query, false); });
        proverPool->wait();

        // For Starting the subsequent stages
        auto nextstage = [=](Query& q) {
            proverPool->post([=]() { compute(q, false); });
        };
        // Check if there are any suspended queries and new answers
        while (cache and cache->hasChanged() and (not query.cb->done)) {
            // New answers have been obtained, resume suspended queries
            cache->next(nextstage);
            proverPool->wait();
        }

        {
            std::lock_guard<std::mutex> lk(lock);
            proverCount--;
        }
        ThreadPoolFactroy::deallocate(proverPool);
        cv.notify_one();
    }

    void Prover::compute(Query query, const bool isLookup)
    {
        if (query.cb->done or done)
            return;  // Done stop searching

        // Found an answer
        if (Clause::EMPTY_CLAUSE(*query)) {
            (*query.cb)(query->getSubstitution(), query.suffix, isLookup);
            return;
        }

        // Instantiate front
        VarSet vset;
        auto* a = (*query->front())(query->getSubstitution(), vset);
        query->front() = a;

        // Check if distinct
        if (a->get_name() == Namer::DISTINCT) {
            computeDistinct(query);
            return;
        }
        // check if or
        if (a->get_type() == Term::OR) {
            computeOr(query);
            return;
        }
        // Check if negation
        if (a->get_type() == Term::NOT) {
            computeNegation(query);
            return;
        }
        auto* atom = (const Atom*)a;
        auto* cache = query.cache;
        // Register query as solution or lookup query
        bool shldCache =
            !contextual(query.context, atom) and cache and !isLookup;
        // don't want to cache true and does, during negations, and lookups
        auto it = shldCache ? (*cache)[query] : Cache::NOT_CACHED;

        if (it.null()) {
            // Solution Node, do an sld-resolution step
            query->pop_front();
            ClauseCB* cb = new ClauseCB(std::move(query));
            compute(atom, query, cb, isLookup);
        } else
            // Lookup node, check if there are answers already computed
            lookup(it, query, atom);
    }

    void Prover::lookup(AnsIterator& it, Query& query, const Atom* lit)
    {
        if (query.cb->done or done)
            return;  // Done stop searching
        /**
         * TODO: CHECK WETHER NOT RENAMING CREATES A PROBLEM
         */
        auto& nxt = it.nxt;
        while (it) {
            auto* soln = *it;
            // do an sld-resolution of lit with each solution
            Substitution mgu;
            // FOR DEBUGGING
            // assert( soln->is_ground() );
            if (Unifier::unifyAtom(*lit, *soln, mgu)) {
                auto resolvent = std::unique_ptr<Clause>(nxt->clone());
                resolvent->setSubstitution(nxt->getSubstitution() + mgu);
                Query nxtQ(resolvent, query.cb, query.context, query.cache,
                           query.suffix, query.random);
                compute(nxtQ, true);
            }
            ++it;
        }
    }

    void Prover::compute(const Atom* lit, Query q, CallBack* cb,
                         const bool lookup)
    {
        if (cb->done or done)
            return;  // Done stop searching
        bool isContxt = contextual(q.context, lit);
        // If its contextual or lit has was head of the resolvent b/n a lookup
        // node and a solution we dont cache
        auto cblit = new LiteralCB(lit, std::unique_ptr<CallBack>(cb),
                                   ((isContxt or lookup) ? nullptr : q.cache));
        std::shared_ptr<CallBack> cbsptr(cblit);
        // Resolve against all program clauses whose heads unify with lit.
        // Need to know weather lit is contextual or not, so as to determine the
        // correct knowldge base to check
        const KnowledgeBase& kb_t = isContxt ? *q.context : *this->kb;
        SuffixRenamer renamer(q.suffix);
        // Resolve against each candidate
        const auto& cs = kb_t[lit->get_name()]->getElements();
        UIterator<const ares::Clause*>* it =
            q.random ? new RandIterator(cs) : new UIterator(cs);
        while (*it) {
            std::unique_ptr<Clause> gn(resolve(lit, ***it, renamer));
            if (gn) {
                Query qn(gn, cbsptr, q.context, q.cache, renamer.gets(),
                         q.random);
                compute(qn, false);
            }
            ++(*it);
        }
        delete it;
    }

    Clause* Prover::resolve(const Atom* lit, const Clause& c, SuffixRenamer& vr)
    {
        auto* mgu = new Substitution();
        VarSet vset;
        auto renamed = (const Atom*)(*c.getHead())(vr, vset);
        if (!Unifier::unifyAtom(*lit, *renamed, *mgu)) {
            delete mgu;
            return nullptr;
        }
        ClauseBody* renamedBody = new ClauseBody(c.size());
        c.renameBody(*renamedBody, vr);
        return new Clause(nullptr, renamedBody, mgu);
    }
    void Prover::computeOr(Query& query)
    {
        const Body& disjuncts = ((const Or*)query->front())->getBody();
        for (auto&& disjunct : disjuncts) {
            query->front() = disjunct;
            std::unique_ptr<Clause> gn(query->clone(true));
            Query nQ(gn, query.cb, query.context, query.cache, query.suffix,
                     query.random);
            compute(nQ, false);
        }
    }
    void Prover::computeNegation(Query& query)
    {
        // front has the form (not α)
        const auto& front = (const Not*)query->front();
        if (not front->is_ground()) {
            query->delayFront();
            compute(query, false);
            return;
        }

        // Get α from
        const Term* alpha = front->getArg(0);

        // Prove <-α
        // Callback which stops all computations when one answer is computed
        std::atomic_bool done = false;
        std::unique_ptr<Clause> g(
            new Clause(nullptr, new Body{alpha}, new Substitution()));
        Query q(
            g, std::shared_ptr<CallBack>(new ClauseCBOne(done, nullptr)),
            query.context, nullptr, 0,
            query.random);  // pstvLit is ground so we could restart the suffix
        compute(q, false);
        // compute(pstvLit, q, new ClauseCBOne(done,nullptr));
        if (done)  // Atleast one answer has been found, so <-α  succedeed
            return;

        // Failed to prove <- α, therefore P |= <-(not) α
        query->pop_front();
        // Continue proving <-α2,..,αn
        compute(query, false);
    }
    void Prover::computeDistinct(Query& query)
    {
        const auto& front = (const Atom*)query->front();
        if (not front->is_ground()) {
            query->delayFront();
            compute(query, false);
            return;
        }

        if (*front->getArg(0) == (*front->getArg(1)))
            return;

        query->pop_front();
        compute(query, false);
    }
}  // namespace ares
