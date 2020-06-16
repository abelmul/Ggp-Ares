#include "utils/gdl/gdlParser/expressionPool.hh"
#include <type_traits>

namespace Ares
{
    template<class T>
    Body* instantiate(const T& expr,const Substitution &sub,VarSet& vSet, bool fn);

    const Term* Function::operator()(const Substitution &sub,VarSet& vSet) const {
        Body* body = instantiate<Function>(*this, sub, vSet, true);
        if( !body ) return nullptr;
        PoolKey key{name, body,true};
        bool exists; 
        auto* fnInst = pool->getFn(key,exists);
        if( exists ) delete body;
        return fnInst;
    }

   const Literal* Literal::operator()(const Substitution &sub,VarSet& vSet) const {
        Body* body = instantiate<Literal>(*this, sub, vSet, true);
        if( !body ) return nullptr;
        PoolKey key{name, body,this->positive};
        bool exists; 
        auto* litInst = pool->getLiteral(key,exists);
        if( exists ) delete body;
        return litInst;
    }

    template<class T>
    Body* instantiate(const T& expr, const Substitution &sub,VarSet& vSet, bool fn){

        Body* body = new Body();
        for (const Term* arg : expr.body)
        {
            const Term* argInst = (*arg)(sub, vSet);
            if( not argInst ){
                delete body;
                return nullptr;     //Must have detected a loop
            }
            body->push_back(argInst);
        }
        /**
         * TODO:
         * Creation and deletion of body could be avoided if Expression pool is 
         * Responsible for the lifecycle of Function Bodies.
         */
        return body;
    }
} // namespace Ares
