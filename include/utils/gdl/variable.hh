#ifndef VARIABLE_HH
#define VARIABLE_HH

#include "utils/gdl/term.hh"

namespace Ares
{
    class Variable : public Term
    {

    public:
        Variable(char* name):Term(name)
        {
            type = VAR;
        }
        bool operator ==(const Variable& y) const{
            return strcasecmp(name,y.name) == 0; 
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
        virtual bool operator==(Term& t)const{
            if(t.getType() != this->type) return false;
            return ( strcasecmp(name, t.getName()) == 0 );
        }
        virtual bool isGround(){
            return false;
        }
        virtual std::string toString(){
            std::ostringstream stringStream;
            stringStream << name;
            #if DEBUG_ARES
            stringStream << "[" << this <<"]";
            #endif
            return stringStream.str();
        }
    };
    
} // namespace Ares

#endif // VARIABLE_HH
