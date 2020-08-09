#include "reasoner/prover.hh"
#include "ares.hh"
namespace ares
{
    Prover* Prover::_prover = nullptr;
    SpinLock Prover::slock;

    void Prover::prove(Query query){
        query.pool = proverPool;                          //This is the initial query
        if( proverPool ){
            proverPool->restart();
            auto f = [&](){
                this->_prove(query);
            };
            proverPool->post(f);               //Schedule an sld tree search to prove this query.
            proverPool->wait();                             //Wait until query is answered.
        }
        else _prove(query);
    }
    
    void Prover::_prove(Query query){
        if( query.done ) return;                            //one answer has been found, No need to keep on searching.
        //Recursively explore the sld tree 
        if( Clause::EMPTY_CLAUSE(*query.goal) ){
            //Successful derivation
            if( query.oneAns ){
                query.done = true;                         //We are done no need to compute more answers.
                if( query.pool ) query.pool->stop();       //No need to schedule further searches.
            }
            query.cb(query.goal->getSubstitution());
            return;
        }

        const cnst_lit_sptr& g = (*query.goal).front();
        if(  g->get_name() == Namer::DISTINCT ){
            bool ok = proveDistinct(*query.goal);    
            if( !ok ) return;          
            //Has Modified query by doing query.goal.pop_front()
            _prove(query);       //Just prove the next goal clause   
            return;
        }
        if( not (*g) ){
            bool ok = handleNegation(*query.goal, query.context,query.renamer);
            if( !ok ) return;
            //Has Modified query by doing query.goal.pop_front()
            _prove(query);       //Just prove the next goal clause   
            return;
        }

        //At this point the goal is positive, try to unify it with clauses in knowledge base.
        const KnowledgeBase* kb_t = contextual(query,g)  ? query.context : this->kb;
        //Get the clauses g might unify with
        const UniqueClauseVec* _clauses = (*kb_t)[g->get_name()];
        if( not _clauses ){
            return;
        }
        // ares::Query<std::function<void(std::reference_wrapper<const ares::Substitution>)> >::Query(const ares::Query<std::function<void(std::reference_wrapper<const ares::Substitution>)> >&);
        Clause* gn;
        for (auto &&c : *_clauses)
        {
            gn = resolve(*query.goal, *c, query.renamer);     //Perfom a normal sld resolution step.
            if(!gn) continue;               //Didn't unify
            //query.goal[0] and head of c unify
            Query q(std::unique_ptr<Clause>(gn), query.context, query.cb, query.oneAns, query.done);  //The next goal clause to prove
            q.pool = query.pool;
            q.renamer.setSuffix( query.renamer.getNxtSuffix());
            //Pool Might not be available due to proving negations.
            if ( not query.pool )
                this->_prove(q);      //no thread available
            else
            {
                auto handler = std::bind(&Prover::_prove, this, q);
                q.pool->post(handler);          //worker threads available
            }
        }
    }
    
    bool Prover::proveDistinct(Clause& goal){
        //goal.front() should be Ground
        VarSet vset;
        cnst_term_sptr l = (*goal.front())(goal.getSubstitution(),vset);
        if ( not l ) return false;      //This happens when a variable is substituted by a term containing it, circular dependency while instantiating.
        if( not l->is_ground() ){
            if( goal.size() == 1 ) 
                throw DistinctNotGround("Distinct called with : (distinct " + l->to_string());
            goal.delayFront();
            return true;
        }
        //Check s != t
        const Literal* lptr = (Literal*)l.get();
        // const Term& s = *l->getArg(0);
        // const Term& t = *l->getArg(1);
        if( (*lptr->getArg(0)) == (*lptr->getArg(1)) ){
            return false;
        }
        goal.pop_front();
        return true;
    }
    bool Prover::handleNegation(Clause& goal,const  State* context, SuffixRenamer& renamer){
        VarSet vset;
        cnst_term_sptr l = (*goal.front())(goal.getSubstitution(),vset);
        if ( not l ) return false;      //This happens when a variable is substituted by a term containing it, circular dependency while instantiating.
        if( not l->is_ground() ){
            if( goal.size() == 1 ) 
                throw NegationNotGround("Negative literal: called with : " + l->to_string());
            goal.delayFront();
            return true;
        }
        //Try to prove <-(not l)  l is negative
        cnst_lit_sptr* _lp = (cnst_lit_sptr*)&l;
        PoolKey key{l->get_name(), new Body(_lp->get()->getBody().begin(),_lp->get()->getBody().end()), true,nullptr};
        cnst_lit_sptr lp = Ares::memCache->getLiteral(key);
        bool success = false;
        bool done = false;              //Shared among threads set to true when an answer is found
        auto _cb = [&success](const Substitution&){
            success = true;         //Just to know if one answer is derived. try to prove if P |= lp,(i.e,  <-lp )
        };
        CallBack cb = _cb;
        Clause* newGoal = new Clause(nullptr, new ClauseBody{lp});
        newGoal->setSubstitution(new Substitution);
        // Query(Clause* _g,const  State* _c , const CallBack<T>& _cb, const bool _one,bool& d)
        Query nQuery(std::unique_ptr<Clause>(newGoal), context, std::ref(cb), true, std::ref(done));
        nQuery.renamer.setSuffix( renamer.getNxtSuffix());
        nQuery.pool = nullptr;
        _prove(nQuery);     //Good old fashioned recursion

        if( success )   //Just proved <- lp meaning we have refuted (<-l) meaning P |= lp. <-l has failed finitely
            return false;
        goal.pop_front();
        return true;            // lp not logical consequence of P, so assume P |=( not lp ) ( equivalently P |= l )
    }
    Clause* Prover::resolve(const Clause& goal, const Clause& c, SuffixRenamer& vr){
        auto vset = VarSet();
        //Make variables in head unique
        cnst_term_sptr _r = (*c.getHead())(vr,vset);
        cnst_lit_sptr* renamedHead = (cnst_lit_sptr*)&_r;
        Substitution* mgu = new Substitution(goal.getSubstitution());
        bool ok = Unifier::unifyPredicate(*goal.front(), **renamedHead, *mgu);
        Clause* gn = nullptr;
        // Ares::mempool->remove(*renamedHead);
        if( not ok ){
            delete mgu;
            return gn;
        }
        ClauseBody* renamedBody = new ClauseBody(c.size() + goal.size() -1 );
        c.renameBody(*renamedBody,vr);
        //Create The resolvent by (goal -{front()} Union c - {c.head} ) 
        gn = new Clause(nullptr, renamedBody);
        gn->insert(c.size(), goal,1);      
        //save the mgu
        gn->setSubstitution(mgu);
        return gn;
    }
} // namespace ares
