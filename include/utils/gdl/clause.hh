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
        Literal* head = nullptr;
        ClauseBody* _body = nullptr;
        ClauseBody& body;
        Context* context = nullptr;

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
        Clause(Literal* head, ClauseBody* _b,Context* c)
        :head(head),_body(_b),body(std::ref(*_body)),context(c)
        {
        };
        /**
         * Create a new renamed clause, used in a resolution step while resolving
         * a goal.
         */ 
        Clause* rename(VarRenamer& vr){
            Context* context = vr.rename(*this->context);
            return new Clause(head, _body, context);
        }
        ~Clause(){
            //A context , and its contents, are unique to a clause            
            //A literal is not unique to just a clause, its shared b/n clauses.
            delete context;
            delete _body;
        }
    };
    
} // namespace Ares

#endif