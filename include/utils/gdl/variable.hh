#ifndef VARIABLE_HH
#define VARIABLE_HH

#include "utils/gdl/term.hh"

namespace Ares
{
    class Variable : public Term
    {
    private:
        Variable(char* name):Term(name,VAR)
        {
        }
    public:
        /**
         * Apply the Substitution sub on this variable, creating an instance.
         * This is done by traversing the "chain" present within the substitution,
         * vset is used to detect any loops. if a variable is encountered more than once 
         * while traversing a chain then there is a loop.
         */
        virtual std::string operator ()(Substitution &sub,VarSet& vSet){
            if( not sub.isBound(this) ) return std::string(name);
            // //There is a circular dependency
            if( vSet.find(this) != vSet.end() ) return std::string();
            // //Remember this var in this particular path
            vSet.insert(this);
            Term* t = sub.get(this);
            std::string tInst = (*t)(sub,vSet);
            // //Done
            vSet.erase(this);
            return tInst;
        }
        virtual bool isGround(){
            return false;
        }
        virtual std::string toString(){
            return std::string(name);
        }
        friend class GdlParser;

    };
    
} // namespace Ares

#endif // VARIABLE_HH
