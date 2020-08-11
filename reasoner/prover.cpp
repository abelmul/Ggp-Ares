#include "reasoner/prover.hh"
#include "ares.hh"
namespace ares
{
    
    Prover* Prover::_prover = nullptr;
    SpinLock Prover::slock;

    std::atomic<int> Query::nextId = 0;
    AnsIterator Cache::NOT_CACHED(nullptr,-1,nullptr);

    void Prover::compute(Query& query){
        proverPool->restart();
        auto* cache = new Cache();
        auto f = [&](){
            compute(query,cache);
        };
        proverPool->post(f);
        proverPool->wait();


        auto fans =[this,cache](AnswerList* ansl){
            auto next = [this,cache](Query& q){
                auto f = [=](){ compute(q,cache); };
                proverPool->post(f);
            };
            ansl->apply(next);
        };
        while (cache->hasChanged())
        {
            proverPool->restart();
            //New answers have been obtained, resume suspended queries
            cache->next(fans);
            proverPool->wait();
        }
        
    }

    void Prover::compute(Query query,Cache* cache){
        //Found an answer
        if( Clause::EMPTY_CLAUSE(*query) )
            (*query.cb)(query->getSubstitution());
        
        if( query.cb->done ) 
            return; //Done stop searching
        
        //Instantiate front
        VarSet vset;
        const cnst_term_sptr& t = (*query->front())(query->getSubstitution(),vset);
        cnst_lit_sptr lit = *(cnst_lit_sptr*)&t;
        query->front() = lit;

        //Check if distinct
        if( lit->get_name() == Namer::DISTINCT ){
            computeDistinct(query,cache);
            return;
        }
        
        //Check if negation
        if( !(*lit) ){
            computeNegation(query,cache);
            return;
        }

        //Register query as solution or lookup query
        bool isCntxt = contextual(query.context, lit); 
        auto it = isCntxt ? Cache::NOT_CACHED : (*cache)[query];    //don't want to cache true and does

        if( it == Cache::NOT_CACHED ){
            //Solution Node, do an sld-resolution step
            query->pop_front();
            ClauseCB* cb = new ClauseCB(std::move(query),cache);
            proverPool->post( [=]{ compute(lit, query.context, cache, cb);} );
        }
        else
            //Lookup node, check if there are answers already computed
            lookup(it, query, lit, cache);
    }

    void Prover::lookup(AnsIterator& it, Query& query,cnst_lit_sptr& lit,Cache* cache){
        /**
         * TODO: CHECK WETHER NOT RENAMING CREATES A PROBLEM
         */
        Clause* nxt = it.nxt;
        for (auto &&soln : it)
        {
            //do an sld-resolution of lit with each solution
            Substitution mgu;
            if( Unifier::unifyPredicate(*lit, *soln,mgu) ) {
                auto resolvent = std::unique_ptr<Clause>(nxt->clone());
                resolvent->setSubstitution( nxt->getSubstitution() + mgu);
                Query nxtQ(resolvent, query.cb,query.context);
                proverPool->post([=] { compute(nxtQ,cache);});
            }
        }
    }
    
    void Prover::compute(cnst_lit_sptr lit,const State* context, Cache* cache, CallBack* cb){

        bool isContxt = contextual(context, lit);
        auto cblit = new LiteralCB(lit, isContxt, std::unique_ptr<CallBack>(cb), cache);
        std::shared_ptr<LiteralCB> cbsptr(cblit);
        //Resolve against all program clauses whose heads unify with lit.
        //Need to know weather lit is contextual or not, so as to determine the correct knowldge base to check
        const KnowledgeBase& kb_t = isContxt ? *context : *this->kb;
        SuffixRenamer renamer;
        for (auto &&c : *kb_t[lit->get_name()])
        {
            std::unique_ptr<Clause> gn( resolve( lit, *c, renamer));
            if( !gn ) continue;
            Query q(gn, cbsptr, context);
            proverPool->post([=]{ compute(q, cache);});
        }
        
    }

    Clause* Prover::resolve(const cnst_lit_sptr& lit, const Clause& c,SuffixRenamer& vr){
        auto* mgu = new Substitution();
        VarSet vset;
        auto r = (*c.front())(vr, vset);
        auto renamed = *(cnst_lit_sptr*)&r;
        if ( !Unifier::unifyPredicate(*lit, *renamed,*mgu) ){
            delete mgu;
            return nullptr;
        }
        ClauseBody* renamedBody = new ClauseBody(c.size());
        c.renameBody(*renamedBody,vr);
        return new Clause( nullptr, renamedBody,mgu);
    }
    void Prover::computeNegation(Query& query,Cache* cache){
        //front has the form (not A1)
        cnst_lit_sptr& front = query->front();
        if( not front->is_ground() ){
            query->delayFront();
            compute(query, cache);
            return;
        }
        
        //Get A1 from memCache
        PoolKey key{front->get_name(), new Body(front->getBody().begin(), front->getBody().end()), true};
        cnst_lit_sptr pstvLit = Ares::memCache->getLiteral(key);

        //Prove <-A1
        std::unique_ptr<Clause> newC(new Clause(nullptr, new ClauseBody{pstvLit}));
        // ClauseCBOne cbOne()
    }
    void Prover::computeDistinct(Query& query,Cache* cache){
        
    }
} // namespace ares
