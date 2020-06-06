#ifndef PROVER_HH
#define PROVER_HH

#include "reasoner/substitution.hh"
#include "utils/gdl/clause.hh"
#include <boost/asio/thread_pool.hpp>
#include <boost/asio/post.hpp>
#include "utils/game/game.hh"

namespace Ares
{ 
    /**
     * TODO: Look into caching
     */
    class Prover
    {
    public:
        CallBack* cb;
        Prover(CallBack* _cb):cb(_cb){};
        /**
         * Carry out backward chaining. Search the sld-tree for a successful refutation.
         * if one == true, the method immediately returns if a single refutation is derived,
         * otherwise all such successful refutations are derived.
         * if it returns true then there's atleast one successful refutation.
         * cb is called each time a successful refutation is derived.
         */
        bool prove(Clause& goal, bool one=true);  
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
    };  
    /**
     * The resulting resolvent,if any, in a single sldnf resolution step.
     */
    struct Resolvent        
    {
        Clause* gn;
        bool ok;
    };
    /**
     * This is used to return the computed answer(s) to the caller in a multi-threaded
     * enviroment.
     */
    struct CallBack
    {
        virtual bool operator()(Substitution* computedAns) = 0;
    };
    
} // namespace Ares

#endif