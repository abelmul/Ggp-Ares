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

namespace ares
{ 
    /**
     * TODO: Look into caching
     */

    struct ClauseHasher
    {
        std::size_t operator()(const Clause& c) const {
            return c.hash();
        }
    };

    struct ClauseEqual{
        bool operator()(const Clause& c1, const Clause& c2) const{
            return c1.hash() == c2.hash();
        }
    };

    template <class T> 
    struct Query{
        Query(Clause* _g,const  State* _c ,T& _cb, const bool _one,bool& d)
        :goal(_g),context(_c), cb(_cb), oneAns(_one),done(d)
        {
        }
        
        void deleteGoal(){ delete goal;}

        Clause* const goal;
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
        std::unordered_map<const Clause, long int , ClauseHasher,ClauseEqual> clauseCount;
        Prover(KnowledgeBase* _kb,ushort proverThreads, ushort negThreads)
        :kb(_kb)
        {
            debug("Prover Threads : " , proverThreads, " negThreads " , negThreads);
            
            if( proverThreads > 0 ) proverPool = new ThreadPool(proverThreads);
            if( negThreads > 0  ) negationPool = new ThreadPool(negThreads);
        };
        
    public:
        static Prover* getProver(KnowledgeBase* _kb,ushort proverThread, ushort negThreads){
            slock.lock();
            if(not  _prover) _prover = new Prover(_kb,proverThread, negThreads);
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
        void setKB(KnowledgeBase* _kb){kb = _kb;}
        
        ~Prover(){
            if(proverPool) delete proverPool;
            if(negationPool) delete negationPool;
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
            bool c = ( ( strcasecmp( g->get_name(), "does") == 0 ) or (strcasecmp( g->get_name(), "true") == 0) ) ;
            return ( c and q.context );
        }

        static Prover* _prover;
        static SpinLock slock;
        KnowledgeBase* kb;
        Substitution* renamer;
        ThreadPool* proverPool = nullptr;     //Used for the initial query, and subequent searches.
        ThreadPool* negationPool = nullptr;   //Used when trying to prove a negative goal(literal).

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
            proverPool->acquire();                          //Need ownership of pool
            auto f = std::bind([&](){
                this->_prove<T>(query);
            });

            proverPool->post<decltype(f)>(f);               //Schedule an sld tree search to prove this query.
            proverPool->wait();                             //Wait until query is answered. Release ownership.
        }
        else _prove(query);
    }

    template<class T>                                       //T is the callback functiions type
    void Prover::_prove(Query<T> query){
        debug("?here");
        debug("\n\n-----_prove called with : -----", query.goal->to_string(), "\n\n");
        if( query.done ) return;                            //one answer has been found, No need to keep on searching.

        //Recursively explore the sld tree 
        if( Clause::EMPTY_CLAUSE(*query.goal) ){
            //Successful derivation
            if( query.oneAns ){
                query.done = true;                         //We are done no need to compute more answers.
                if( query.pool ) query.pool->stop();       //No need to schedule further searches.
            }
            query.cb(query.goal->getSubstitution());
            delete query.goal;
            return;
        }

        const cnst_lit_sptr& g = (*query.goal).front();

        if( strcasecmp( g->get_name(), "distinct") == 0 ){
            bool ok = proveDistinct(*query.goal);    
            if( !ok ) return;          
            //Has Modified query by doing query.goal.pop_front()
            _prove(query);       //Just prove the next goal clause   
            return;
        }

        if( not (*g) ){
            // clauseCount[*query.goal]++;
            bool ok = handleNegation(*query.goal, query.context,query.renamer);
            if( !ok ) return;
            //Has Modified query by doing query.goal.pop_front()
            _prove(query);       //Just prove the next goal clause   
            return;
        }

        //At this point the goal is positive, try to unify it with clauses in knowledge base.
        const KnowledgeBase* kb_t = contextual(query,g)  ? query.context : this->kb;
        // if( not contextual(query,g) ) clauseCount[*query.goal]++;
        //Get the clauses g might unify with
        const std::vector<const Clause*>* _clauses = (*kb_t)[g->get_name()];
        if( not _clauses ){
            query.deleteGoal();     //Won't ever use goal again.
            return;
        }
        
        Resolvent g_n;
        debug("\n\n----Beginning of for loop :----" , query.goal->to_string() ,"\n");
        for (auto &&c : *_clauses)
        {
            g_n = resolve(*query.goal, *c, query.renamer);     //Perfom a normal sld resolution step.
            debug( "after resolve");
            if(!g_n.ok) continue;               //Didn't unify
            //query.goal[0] and head of c unify
            Query<T> q(g_n.gn, query.context, query.cb, query.oneAns, query.done);  //The next goal clause to prove
            q.pool = query.pool;
            q.renamer.setSuffix( query.renamer.getNxtSuffix());
            debug( "after resolve2");
            //Pool Might not be available due to proving negations.
            auto handler = std::bind([&, q](){
                this->_prove(q);
            });
            debug( "after resolve3");

            if ( not q.pool ){
                debug("no pool");
                this->_prove<T>(q);
            }

            else
                q.pool->template post<decltype(handler)>(handler);
            debug( "after resolve5");

        }
        debug("-----End of for loop :----" , query.goal->to_string() , "\n\n");
        delete query.goal;
    }

} // namespace ares

#endif