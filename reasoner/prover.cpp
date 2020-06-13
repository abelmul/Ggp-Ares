#include "reasoner/prover.hh"

namespace Ares
{
    void Prover::wait(Query* query) {
        if( query->oneAns ){
            auto f = std::bind( [query](){
                //Wait until an answer has been computed.
                query->waitOne();
                query->done(true);        //We are done no need to compute more answers.
                /**
                 * This isn't the owning thread need to call this from one of the worker threads.
                 * query->pool()->stop();    //No need to schedule further searches.
                 */
            });
            waitingThreads->post<decltype(f)>(f);
        }
        query->pool()->wait();       //Wait untill all jobs/searches submitted are finished.
    }
    
    void Prover::prove(Query* query){
        query->pool(proverPool);
        proverPool->acquire();                  //Need ownership of pool
        auto f = std::bind(&Prover::_prove, this, query);
        proverPool->post<decltype(f)>(f);       //Schedule an sld tree search to prove this query.
        wait(query);                            //Wait until query is answered. Release ownership.
    }

    void Prover::_prove(Query* query){
        //Recursively explore the sld tree 
        if( Clause::EMPTY_CLAUSE(*query->goal) ){
            //Successful derivation
        }

        const Literal& g = (*query->goal)[0];
        if( not g ){
            //Handle negation
            return;
        }
        if( strcasecmp( g.getName(), "distinct") == 0 ){
            //Handle distinct
            return;
        }

        //At this point the goal is positive, try to unify it with clauses in knowledge base.
        KnowledgeBase* kb_t = nullptr;
        if( strcasecmp( g.getName(), "true") == 0){}
        if( strcasecmp( g.getName(), "does") == 0){}

    }
} // namespace Ares
