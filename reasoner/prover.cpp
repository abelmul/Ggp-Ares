#include "reasoner/prover.hh"

namespace Ares
{
    Prover* Prover::_prover = nullptr;
    SpinLock Prover::slock;
    
    
    bool Prover::proveDistinct(Clause& goal){
        //goal.front() should be Ground
        const Literal& l = goal.front();
        if( not l.isGround() ){
            if( goal.size() == 1 ) 
                throw DistinctNotGround("Distinct called with : (distinct " + l.toString());
            goal.delayFront();
            return true;
        }

        //Check s != t
        const Term& s = *l.getArg(0);
        const Term& t = *l.getArg(1);
        if( s == t ) return false;
        
        goal.popFront();
        return true;
    }
    bool Prover::handleNegation(Clause& goal,const  State* context){
        const Literal& l = goal.front();

        if( not l.isGround() ){
            if( goal.size() == 1 ) 
                throw NegationNotGround("Negative literal: called with : " + l.toString());
            goal.delayFront();
            return true;
        }
        //Try to prove <-(not l)  l is negative
        PoolKey key{l.getName(), l.getBody(), true};
        bool exists;
        const Literal* lp = l.pool->getLiteral(key,exists);


        bool success = false;
        bool done = false;              //Shared among threads set to true when an answer is found
        auto cbL = [&success](const Substitution&){
            success = true;         //Just to know if one answer is derived. try to prove if P |= lp,(i.e,  <-lp )
        };
        CallBack<decltype(cbL)> cb(cbL);
        Clause* newGoal = new Clause(nullptr, new ClauseBody{lp});
        // Query(Clause* _g,const  State* _c , const CallBack<T>& _cb, const bool _one,bool& d)
        Query<decltype(cbL)> nQuery(newGoal, context, std::ref(cb), true, std::ref(done));
        auto handler = std::bind([&](){
            this->_prove<decltype(cbL)>(nQuery);
        });
        if( negationPool->try_acquire() ){
            //Acuquired thread pool
            nQuery.pool = negationPool;
            negationPool->post<decltype(handler)>(handler);
            negationPool->wait();
        }
        else _prove<decltype(cbL)>(nQuery);     //Good old fashioned recursion

        if( success )   //Just proved <- lp meaning we have refuted (<-l) meaning P |= lp. <-l has failed finitely
            return false;
        
        goal.popFront();
        return true;            // lp not logical consequence of P, so assume P |=( not lp ) ( equivalently P |= l )
    }
    
    Resolvent Prover::resolve(const Clause& goal, const Clause& c){
        Clause* renamed = c.rename();      //Make variable unique
        Substitution* mgu = new Substitution();
        bool ok = Unifier::unifyPredicate(goal.front(), c.getHead(), *mgu);
        Resolvent gn;
        gn.ok = ok;
        if( not ok ) return gn;

        renamed->setSubstitution(mgu);
        renamed->setHead(nullptr);
        (*renamed) += goal;     //The resolvent, the next goal clause
        gn.gn = renamed;
        return gn;
    }
} // namespace Ares
