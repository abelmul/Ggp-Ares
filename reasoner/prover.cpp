#include "reasoner/prover.hh"
#include "ares.hh"

namespace ares
{
    Prover* Prover::_prover = nullptr;
    SpinLock Prover::slock;
    
    
    bool Prover::proveDistinct(Clause& goal){
        //goal.front() should be Ground
        VarSet vset;
        cnst_term_sptr l = (*goal.front())(goal.getSubstitution(),vset);
        if ( not l ) return false;      //This happens when a variable is substituted by a term containing it, circular dependency while instantiating.

        if( not l->is_ground() ){
            if( goal.size() == 1 ) 
                throw DistinctNotGround("Distinct called with : (distinct " + l->to_string());
            goal.delayFront();
            Ares::mempool->remove(l);
            return true;
        }

        //Check s != t
        const Literal* lptr = (Literal*)l.get();
        // const Term& s = *l->getArg(0);
        // const Term& t = *l->getArg(1);
        if( (*lptr->getArg(0)) == (*lptr->getArg(1)) ){
            Ares::mempool->remove(l);
            return false;
        }
        Ares::mempool->remove(l);
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
        PoolKey key{l->get_name(), new Body(_lp->get()->getBody().begin(),_lp->get()->getBody().end()), true};
        cnst_lit_sptr lp = Ares::exprpool->getLiteral(key);

        bool success = false;
        bool done = false;              //Shared among threads set to true when an answer is found
        auto cb = [&success](const Substitution&){
            success = true;         //Just to know if one answer is derived. try to prove if P |= lp,(i.e,  <-lp )
        };
        Clause* newGoal = new Clause(nullptr, new ClauseBody{lp});
        newGoal->setSubstitution(new Substitution);
        // Query(Clause* _g,const  State* _c , const CallBack<T>& _cb, const bool _one,bool& d)
        Query<decltype(cb)> nQuery(newGoal, context, std::ref(cb), true, std::ref(done));
        nQuery.renamer.setSuffix( renamer.getNxtSuffix());
        auto handler = std::bind([&](){
            this->_prove<decltype(cb)>(nQuery);
        });

        nQuery.pool = negationPool;
        if( negationPool and negationPool->try_acquire() ){
            //Acuquired thread pool
            negationPool->post<decltype(handler)>(handler);
            negationPool->wait();
        }
        else
            _prove<decltype(cb)>(nQuery);     //Good old fashioned recursion
        
        Ares::mempool->remove(l);
        Ares::mempool->remove(lp);

        if( success )   //Just proved <- lp meaning we have refuted (<-l) meaning P |= lp. <-l has failed finitely
            return false;
        
        goal.pop_front();
        return true;            // lp not logical consequence of P, so assume P |=( not lp ) ( equivalently P |= l )
    }
    
    Resolvent Prover::resolve(const Clause& goal, const Clause& c, SuffixRenamer& vr){
        auto vset = VarSet();
        //Make variables in head unique
        cnst_term_sptr _r = (*c.getHead())(vr,vset);
        debug("c is ", c.to_string());
        cnst_lit_sptr* renamedHead = (cnst_lit_sptr*)&_r;
        Substitution* mgu = new Substitution(goal.getSubstitution());
        bool ok = Unifier::unifyPredicate(goal.front(), *renamedHead, *mgu);
        Resolvent gn;
        gn.ok = ok;
        Ares::mempool->remove(*renamedHead);
        if( not ok ){
            delete mgu;
            return gn;
        }
        debug("c is ", c.to_string());
        debug( "Size is ", c.size(), " Goal size is ", goal.size());
        ClauseBody* renamedBody = new ClauseBody(c.size() + goal.size() -1 );
        if( c.size() > 0) debug( "arity [0] is ", c.front()->to_string()," arity ",c.front()->getArity());
        c.renameBody(*renamedBody,vr);
        debug("?here?1 ");
        //Create The resolvent by (goal -{front()} Union c - {c.head} ) 
        Clause* renamed = new Clause(nullptr, renamedBody);
        renamed->insert(c.size(), goal,1);      
        debug("?here?");
        //save the mgu
        debug("renamed ", renamed->to_string());
        renamed->setSubstitution(mgu);
        gn.gn = renamed;
        
        return gn;
    }
} // namespace ares
