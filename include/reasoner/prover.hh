#ifndef PROVER_HH
#define PROVER_HH

#include "reasoner/substitution.hh"
#include "utils/gdl/clause.hh"
#include <condition_variable>
#include "utils/game/match.hh"
#include "utils/threading/threadPool.hh"
#include "utils/utils/exceptions.hh"
#include "utils/gdl/gdlParser/gdlParser.hh"
#include "utils/memory/expressionPool.hh"
#include "reasoner/unifier.hh"
#include "utils/utils/cfg.hh"
#include "utils/threading/loadBalancer.hh"

namespace ares
{ 
    /**
     * TODO: Look into caching
     */

    template <class T> 
    struct Query{
        Query(Clause* _g,const  State* _c ,T& _cb, const bool _one,bool& d)
        :goal(_g),context(_c), cb(_cb), oneAns(_one),done(d)
        {
        }
        void deleteGoal(){ delete goal; goal = nullptr;}
        Clause* goal;
        const State* const context;
        T& cb;                    //This is used to return the computed answer(s) to the caller in a multi-threaded enviroment.
        ThreadPool* pool = nullptr;
        SuffixRenamer renamer;
        const bool oneAns;
        bool& done;
    };
    
    /**
     * The resulting resolvent,if any, in a single sldnf resolution step.
     */
    struct Resolvent        
    {
        Clause* gn;
        bool ok;
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
         * Carry out backward chaining. Search the sld-tree for a successful refutation.
         * Using kb + @param state as a combined knowledgebase.
         * if one == true, the method immediately returns if a single refutation is derived,
         * otherwise all such successful refutations are derived.
         * cb is called each time a successful refutation is derived.
         */
        template <class T>
        void prove(Query<T>& query);  
        void setKB(const KnowledgeBase* _kb){kb = _kb;}
        ~Prover(){
            if(proverPool) delete proverPool;
        }
    private:
        /**
         * Extension of prove(Query<T>& query), needed for recursion.
         */ 
        template <class T>
        void _prove(Query<T> query);
        /**
         * Carry out a single (normal) SLD-Resolution Step
         * Where the selected literal in goal is positive.
         */
        Resolvent resolve(const Clause& goal, const Clause& c,SuffixRenamer& vr);
        /**
         * Carry out a single SLDNF-Resolution Step
         * Where the selected literal in goal is negative.
         */
        bool handleNegation(Clause& goal, const State* context,SuffixRenamer& renamer);
        /**
         * The distinct relation.
         */
        bool proveDistinct(Clause& goal);
        template <class T>
        inline bool contextual(const Query<T>& q, const cnst_lit_sptr& g) const{
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

    /**
     * Implementation for template methods of Prover.
     */
    template<class T>                                   //T is the callback functions type
    void Prover::prove(Query<T>& query){
        query.pool = proverPool;                          //This is the initial query
        if( proverPool ){
            proverPool->restart();
            auto f = [&](){
                this->_prove<T>(query);
            };
            proverPool->post(f);               //Schedule an sld tree search to prove this query.
            proverPool->wait();                             //Wait until query is answered.
        }
        else _prove(query);
    }
    
    template<class T>                                       //T is the callback functiions type
    void Prover::_prove(Query<T> query){
        if( query.done ) return;                            //one answer has been found, No need to keep on searching.
        //Recursively explore the sld tree 
        if( Clause::EMPTY_CLAUSE(*query.goal) ){
            //Successful derivation
            if( query.oneAns ){
                query.done = true;                         //We are done no need to compute more answers.
                if( query.pool ) query.pool->stop();       //No need to schedule further searches.
            }
            query.cb(query.goal->getSubstitution());
            query.deleteGoal();
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
            query.deleteGoal();     //Won't ever use goal again.
            return;
        }

        Resolvent g_n;
        for (auto &&c : *_clauses)
        {
            g_n = resolve(*query.goal, *c, query.renamer);     //Perfom a normal sld resolution step.
            if(!g_n.ok) continue;               //Didn't unify
            //query.goal[0] and head of c unify
            Query<T> q(g_n.gn, query.context, query.cb, query.oneAns, query.done);  //The next goal clause to prove
            q.pool = query.pool;
            q.renamer.setSuffix( query.renamer.getNxtSuffix());
            //Pool Might not be available due to proving negations.
            auto handler = [&, q](){
                this->_prove(q);
            };
            if ( not q.pool ){
                this->_prove<T>(q);
            }
            else
                q.pool->post(handler);
        }
        query.deleteGoal();
    }
} // namespace ares
#endif