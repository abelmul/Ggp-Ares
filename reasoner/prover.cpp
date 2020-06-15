#include "reasoner/prover.hh"

namespace Ares
{
    template<class T>
    void Prover<T>::prove(Query<T>& query){
        query.pool = proverPool;                         //This is the initial query
        proverPool->acquire();                          //Need ownership of pool
        auto f = std::bind(&Prover::_prove, this, std::ref(query));
        proverPool->post<decltype(f)>(f);               //Schedule an sld tree search to prove this query.
        proverPool->wait();                             //Wait until query is answered. Release ownership.
    }

    template<class T>
    void Prover<T>::_prove(Query<T> query){
        if( query.done ) return;      //one answer has been found, No need to keep on searching.
        //Recursively explore the sld tree 
        if( Clause::EMPTY_CLAUSE(*query.goal) ){
            //Successful derivation
            if( query.oneAns ){
                query.done = true;                           //We are done no need to compute more answers.
                query.pool->stop();                       //No need to schedule further searches.
            }
            (*query.cb)(query.goal->getSubstitution());     //Call the registered call back
            return;
        }

        const Literal& g = (*query.goal).front();

        if( strcasecmp( g.getName(), "distinct") == 0 ){
            bool ok = proveDistinct(*query.goal);    
            if( !ok ) return;          
            //Has Modified query by doing query.goal.popFront()
            _prove(query);       //Just prove the next goal clause   
            return;
        }

        if( not g ){
            bool ok = handleNegation(*query);
            if( !ok ) return;
            //Has Modified query by doing query.goal.popFront()
            _prove(query);       //Just prove the next goal clause   
            return;
        }

        //At this point the goal is positive, try to unify it with clauses in knowledge base.
        KnowledgeBase& kb_t;
        if( contextual(query,g) ) 
            kb_t = *query.context;
        else
             kb_t = *this->kb;
        
        //Get the clauses g might unify with
        const std::vector<const Clause*> clauses = kb_t[g.getName()];
        Resolvent g_n;
        for (auto &&c : clauses)
        {
            g_n = resolve(*query.goal, *c);     //Perfom a normal sld resolution step.
            if(!g_n.ok) continue;
            //query.goal and head of c unify
            Query<T> q(g_n.gn, query.context, query.cb, query.oneAns, query.done);
            q.pool = query.pool;
            //Pool Might not be available due to proving negations.
            auto handler = std::bind(&Prover::_prove, this, q );
            if ( not q.pool ) handler();
            else q.pool->template post<decltype(handler)>(handler);
        }
    }

    template<class T>
    bool Prover<T>::proveDistinct(Clause& goal){
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
        if(not (s == t) ){
            goal.popFront();
            return true;
        }
        return false;
    }
    template<class T>
    bool Prover<T>::handleNegation(Query<T>& query){
        Clause& goal = *query.goal;
        const Literal& l = goal.front();
        if( not l.isGround() ){
            if( goal.size() == 1 ) 
                throw NegationNotGround("Negative literal: called with : " + l.toString());
            goal.delayFront();
            return true;
        }
        PoolKey key{l.getName(), l.getBody(), true};
        bool exists;
        const Literal* lp = l.pool->getLiteral(key,exists);
        bool success = false;
        bool done = false;
        auto cbL = [&success](Substitution&){
            success = true;         //Just to know if one answer is derived. try to prove if P |= lp,(i.e <-lp)
        };
        CallBack<decltype(cbL)> cb(cbL);
        Clause* newGoal = new Clause(nullptr, new ClauseBody{lp});
        // Query(Clause* _g,const  State* _c , const CallBack<T>* _cb, const bool _one,bool& d)
        Query<T> nQuery(newGoal, query.context, &cb, true, &done);
        auto handler = std::bind( &Prover::_prove,this,nQuery );
        ThreadPool* tp = query.pool;
        if( tp->try_acquire() ){
            //Acuquired thread pool
            tp->post<decltype(handler)>(handler);
            tp->wait();
        }
        else handler();     //Good old fashion recursion

        if( success )   //Just proved <- lp meaning we have refuted (not lp) meaning P |= lp.
            return false;
        
        delete newGoal;
        goal.popFront();
        return true;
    }
    template<class T>
    Resolvent Prover<T>::resolve(const Clause& goal, const Clause& c){
        return Resolvent();
    }
} // namespace Ares
