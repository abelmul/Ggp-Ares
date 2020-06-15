#ifndef CONST_HH
#define CONST_HH

#include "utils/gdl/term.hh"
#include <iostream>

namespace Ares
{
    class Constant: public Term
    {
    friend class ExpressionPool;
    friend class ExpressionPoolTest;

    private:
        ~Constant(){
            delete name;
        }
        
    public:
        Constant(const char* name):Term(name,CONST){}
        virtual const Term* operator ()(Substitution &sub,VarSet& vStack) const {
            return this;
        }
        virtual bool isGround() const {
            return true;
        }
        virtual std::string toString() const{
            return std::string(name);
        }
        virtual std::size_t hash() const {
            return nameHasher(name);
        }
    };
    
} // namespace Ares

#endif