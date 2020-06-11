#ifndef PROVER_HH
#define PROVER_HH

#include "reasoner/substitution.hh"
#include "utils/gdl/clause.hh"
#include <boost/asio/thread_pool.hpp>
#include <condition_variable>
#include <boost/asio/post.hpp>
#include "utils/game/match.hh"
namespace Ares
{ 
    /**
     * TODO: Look into caching
     */

    /**
     * This is used to return the computed answer(s) to the caller in a multi-threaded
     * enviroment.
     */
    struct CallBack
    {
        virtual bool operator()(Substitution* computedAns) = 0;
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
    public:
        CallBack* cb;
        Prover(CallBack* _cb):cb(_cb){};
        /**
         * Carry out backward chaining. Search the sld-tree for a successful refutation.
         * Using kb + @param state as a combined knowledgebase.
         * if one == true, the method immediately returns if a single refutation is derived,
         * otherwise all such successful refutations are derived.
         * cb is called each time a successful refutation is derived.
         */
        bool prove(Clause& goal, State* state, bool one=true);  
        static void setKB(KnowledgeBase* _kb){kb = _kb;}
    private:
        static KnowledgeBase* kb;
        /**
         * Carry out a single (normal) SLD-Resolution Step
         */
        Resolvent* resolve(Clause& goal, Clause& c);
        /**
         * Carry out a single SLDNF-Resolution Step
         * Where the selected literal in goal is negative.
         */
        Resolvent* handleNegation(Clause& goal);
        /**
         * The distinct relation.
         */
        bool distinct(Term& s, Term& t){
            return not ( s == t);
        }

        std::mutex lock;
        std::condition_variable done;   
    };  
    
} // namespace Ares

#endif