#ifndef VARIABLE_HH
#define VARIABLE_HH

#include "utils/gdl/term.hh"

namespace Ares
{
    class Variable : public Term
    {
    
    friend class ExpressionPool;
    friend class ExpressionPoolTest;

    private:
        Variable(const char* name):Term(name,VAR)
        {
        }
        /*Managed By ExpressionPool*/
        ~Variable(){
            delete name;
        }
    public:
        /**
         * Apply the Substitution sub on this variable, creating an instance.
         * This is done by traversing the "chain" present within the substitution,
         * vset is used to detect any loops. if a variable is encountered more than once 
         * while traversing a chain then there is a loop.
         */
        virtual const Term* operator ()(const Substitution &sub,VarSet& vSet) const {
            if( not sub.isBound(this) ) return this;
            else if(sub.isRenaming() ) return sub[this];        //No need to traverse the chain

            if( vSet.find(this) != vSet.end() ) return nullptr;     //There is a circular dependency
            // //Remember this var in this particular path
            vSet.insert(this);
            const Term* t = sub.get(this);
            const Term* tInst = (*t)(sub,vSet);
            // //Done
            vSet.erase(this);   
            return tInst;
        }
        virtual bool isGround() const{
            return false;
        }

        virtual std::size_t hash() const{
            return nameHasher(name);
        }

        virtual std::string toString() const {
            return std::string(name);
        }
    };
    
} // namespace Ares

#endif // VARIABLE_HH
