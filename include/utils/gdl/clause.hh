#ifndef CLAUSE_HH
#define CLAUSE_HH

#include "utils/gdl/literal.hh"
#include "reasoner/varRenamer.hh"

namespace Ares
{
    typedef std::vector<Literal*> ClauseBody;

    class Clause
    {
    private:
        const Literal* head = nullptr;
        const ClauseBody* _body = nullptr;
        const ClauseBody& body;
        const Substitution* theta = nullptr;

    public:
        /**
         * Protect against accidental copying, pass by value,...
         */
        Clause(const Clause& c) = delete;
        Clause& operator =(const Clause& c) = delete;

        /**
         * A clause has this kind of form:
         * A <- A0 and ... and An
         * where A0...An are literals(the body), and A is the head
         */
        Clause(Literal* head, const ClauseBody* _b)
        :head(head),_body(_b),body(std::ref(*_body))
        {
        };
        /**
         * Create a new renamed clause, used in a resolution step while resolving
         * a goal.
         */ 
        Clause* rename(VarRenamer& vr){
            return nullptr;
        }
        const ClauseBody* getBody(){return _body;}
        ~Clause(){
            //A literal is not unique to just a clause, its shared b/n clauses, managed by ExpressionPool.
            if (theta) delete theta;
            if( body.size() != 0) delete _body; //Don't delete gdlParser::EMPTY_BODY;
        }
    };
    
} // namespace Ares

#endif