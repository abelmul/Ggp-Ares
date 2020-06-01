#ifndef VARIABLE_HH
#define VARIABLE_HH

#include "utils/gdl/term.hh"

namespace Ares
{
    class Variable : public Term
    {

    public:
        Variable(std::string name):Term(name)
        {
        }
        /**
         * Apply Substitution sub on this variable, return its replacement if bound 
         * otherwise return self.
         */
        virtual Term* operator ()(Substitution &sub,bool inPlace=false){
            if( sub.isBound(this) )
                return (*sub[this])(EMPTY_SUB); //Return a new instance of the term its bound to

            return this;
        }
        
        virtual bool isGround(){
            return false;
        }
        //Used for resolving hash collisions in Substitutions.
        bool operator ==(const Variable &y)const{
            return y.name == name;
        }
        virtual std::string toString(){
            std::ostringstream stringStream;
            stringStream << name;
            // stringStream << name << "[" << this <<"]";
            return stringStream.str();
        }
    };
    
} // namespace Ares

#endif // VARIABLE_HH
