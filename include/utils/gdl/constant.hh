#ifndef CONST_HH
#define CONST_HH

#include "utils/gdl/term.hh"
#include <iostream>

namespace Ares
{
    class Constant: public Term
    {
    public:
        Constant(char* name):Term(name){ type = CONST;}
        virtual Term* operator ()(Substitution &sub,bool inPlace=false){
            return this;
        }
        virtual bool isGround(){
            return true;
        }
        bool operator == (const Constant& c) const{
            return strcasecmp(name,c.name) == 0;
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

#endif