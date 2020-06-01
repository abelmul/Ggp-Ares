#ifndef CONST_HH
#define CONST_HH

#include "utils/gdl/term.hh"
#include <iostream>

namespace Ares
{
    class Constant: public Term
    {
    public:
        Constant(std::string name):Term(name){}
        virtual Term* operator ()(Substitution &sub,bool inPlace=false){
            return this;
        }
        virtual bool isGround(){
            return true;
        }
        virtual std::string toString(){
            std::ostringstream stringStream;
            stringStream << name;
            // stringStream << name << "[" << this <<"]";
            return stringStream.str();
        }
    };
    
} // namespace Ares

#endif