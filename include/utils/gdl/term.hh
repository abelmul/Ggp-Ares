#ifndef TERM_HH
#define TERM_HH

#include <string>
#include <string.h>
#include <vector>
#include <iostream>

#include <sstream>
#include "reasoner/substitution.hh"
#include <unordered_set>
#include <stack>
namespace Ares
{
    enum Type {VAR,CONST,FN};
    class GdlParser;
    typedef std::unordered_set<Variable*,VarHasher,VarEqual> VarSet;
    struct VarStack
    {
        void push(Variable* x){
            stack.push(x);
            varSet.insert(x);   
        }
        void pop(){

        }
        bool contains(Variable* x){
            return varSet.find(x) != varSet.end();
        }
        private:
            std::unordered_set<Variable*,VarHasher,VarEqual> varSet;
            std::stack<Variable*> stack;
    };
    
    //Variables, functions, constants all inherit from this abstract class.
    class Term
    {
    protected:
        char* name;
        Type type;
        Term(char* n,Type t):name(n),type(t){}
        
    public:
        /**
         * Use Term.operator()(Substitution sub) to create a deep clone.
         * Protect against accidental copying,assignment, and return by value.
        */
        Term(const Term &t) = delete;
        Term& operator = (const Term &t) = delete;
        /**
         * Apply the Substitution sub on this term, creating an instance.
         * This is done by traversing the "chain" present within the substitution,
         * Varset is used to detect any loops. if a variable is encountered more than once then
         * there is a loop.
         */
        virtual std::string operator ()(Substitution &sub,VarSet& vSet) = 0;
        virtual bool isGround() = 0;
        virtual bool operator==(Term& t) const{
            //They are equal iff they have the same address.
            //Only one instance of a term exists.
            return this == &t;
        };

        char* getName() const {return name;}
        Type getType(){return type;}
        virtual std::string toString() = 0;
        virtual ~Term(){}

        friend class GdlParser;
    };
    #define isVar(t)  (t.getType() == VAR)
    #define isConst(t)  (t.getType() == CONST)
    #define isFn(t)  (t.getType() == FN)
} // namespace Ares

#endif