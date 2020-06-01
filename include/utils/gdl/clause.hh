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
        ~Clause();
    };
    
    Clause::~Clause()
    {
        //A context , and its contents, are unique to a clause
        for (auto &it : context->getMapping())
            if(it.second->deleteable)
                delete it.second;
        
        delete context;
        
        //A literal is not unique to just a clause, its shared b/n clauses.
        delete _body;
    }
    
} // namespace Ares

#endif