#ifndef CONST_HH
#define CONST_HH

#include "utils/gdl/term.hh"
#include <iostream>

namespace Ares
{
    class Constant: public Term
    {
    friend class ExpressionPool;
    private:
        ~Constant(){
            delete name;
        }
        
    public:
        Constant(const char* name):Term(name,CONST){}
        virtual std::string operator ()(Substitution &sub,VarSet& vStack){
            return std::string(name);
        }
        virtual bool isGround(){
            return true;
        }
        virtual std::string toString(){
            return std::string(name);
        }
        virtual std::size_t hash() const {
            return nameHasher(name);
        }
    };
    
} // namespace Ares

#endif