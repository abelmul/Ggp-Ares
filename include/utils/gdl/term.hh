#ifndef TERM_HH
#define TERM_HH

#include <string>
#include <string.h>
#include <vector>
#include <iostream>
#include <sstream>
#include <unordered_set>
#include <stack>
#include "reasoner/substitution.hh"
#include "utils/hashing.hh"

namespace Ares
{
    enum Type {VAR,CONST,FN};
    class SymbolsPool;
    typedef std::unordered_set<Variable*,VarHasher,VarEqual> VarSet;

    //Variables, functions, constants all inherit from this abstract class.
    class Term
    {
    friend class ExpressionPool;
    
    protected:
        const char* const name;

        Type type;
        Term(const char* n,Type t):name(n),type(t){}
        virtual ~Term(){}

    public:
        static CharpHasher nameHasher;  
        /**
         * Use Term.operator()(Substitution sub) to create a deep clone.
         * Protect against accidental copying,assignment, and return by value.
        */
        Term(const Term&) = delete;
        Term(const Term&&) = delete;
        Term& operator=(Term&&) = delete;
        Term& operator = (const Term&) = delete;
        /**
         * Apply the Substitution sub on this term, creating an instance.
         * This is done by traversing the "chain" present within the substitution,
         * Varset is used to detect any loops. if a variable is encountered more than once then
         * there is a loop.
         */
        virtual std::string operator ()(Substitution &sub,VarSet& vSet) = 0;
        virtual bool isGround() = 0;
        virtual std::size_t hash() const = 0;
        virtual bool operator==(Term& t) const{
            //They are equal iff they have the same address.
            //Only one instance of a term exists.
            return this == &t;
        };

        const char* getName() const {return name;}
        Type getType(){return type;}
        virtual std::string toString() const = 0;
        friend std::ostream & operator << (std::ostream &out, const Term &t){
            out << t.toString();
            return out;
        }
    };
    
    inline void hash_combine(std::size_t& seed, Term* v) {
        std::size_t hash = v->hash();
        seed ^= hash + 0x9e3779b9 + (seed<<6) + (seed>>2);
    }

    #define isVar(t)  (t.getType() == VAR)
    #define isConst(t)  (t.getType() == CONST)
    #define isFn(t)  (t.getType() == FN)
} // namespace Ares

#endif  