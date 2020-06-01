#ifndef TERM_HH
#define TERM_HH

#include <string>
#include <vector>
#include <iostream>
#include <sstream>
#include "reasoner/substitution.hh"

namespace Ares
{
    //Variables, functions, constants all inherit from this abstract class.
    class Term
    {
    protected:
        std::string name;

    public:
        Term(std::string n):name(n){}
        Term(std::string n,bool d):name(n),deleteable(d){}
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
        std::string getName() const {return name;}
        virtual std::string toString() = 0;

        bool deleteable = false;
        ~Term(){}
    };  
} // namespace Ares

#endif