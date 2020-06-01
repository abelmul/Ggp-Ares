#ifndef TERM_HH
#define TERM_HH

#include <string>
#include <string.h>
#include <vector>
#include <iostream>

#include <sstream>
#include "reasoner/substitution.hh"

namespace Ares
{
    enum Type {VAR,CONST,FN};
    //Variables, functions, constants all inherit from this abstract class.
    class Term
    {
    protected:
        char* name;
        Type type;

    public:
        Term(char* n):name(n){}
        Term(char* n,bool d):name(n),deleteable(d){}
        /**
         * Use Term.operator()(Substitution sub) to create a deep clone.
         * Protect against accidental copying,assignment, and return by value.
        */
        Term(const Term &t) = delete;
        Term& operator = (const Term &t) = delete;
        /**
         * Apply Substitution sub on this term.
         * if inplace=false, then no new term is created the substitution 
         * is done inplace.
         * else a new term is created and that is modified using sub.
         */
        virtual Term* operator ()(Substitution &sub,bool inPlace=false) = 0;
        virtual bool isGround() = 0;
        char* getName() const {return name;}
        Type getType(){return type;}
        virtual std::string toString() = 0;

        bool deleteable = false;
        virtual ~Term(){
            free(name);
        }
    };
    #define isVar(t)  (t.getType() == VAR)
    #define isConst(t)  (t.getType() == CONST)
    #define isFn(t)  (t.getType() == FN)
} // namespace Ares

#endif