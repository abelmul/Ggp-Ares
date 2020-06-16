#ifndef SUBST_HH
#define SUBST_HH

#include <unordered_map>
#include <functional>
#include <string>
#include "utils/hashing.hh"

namespace Ares
{

    class Variable;
    class Term;
    
    typedef std::unordered_map<const Variable*,const Term*,VarHasher,VarEqual> Mapping;

    class Substitution
    {

    protected:
        Mapping mappping;

    public:
        /**
         * Protect against accidental copying, pass by value, ...
         */
        Substitution(const Substitution& s) = delete;
        Substitution& operator = (const Substitution& other) = delete;

        Substitution(){}
        static Substitution emptySub;

        bool bind(const Variable* x, const Term* t);

        //To get the immediate mapping, without traversing the chain.
        const Term* get(const Variable* x) const ;
        //Overload the indexing operator, to get the underlying exact mapping        
        const Term* operator[]  (const Variable* x) const;

        bool isRenaming() const { return false;}
        /**
         * Check if this variable is bound
         */ 
        bool isBound(const Variable* x) const;
        /**
         * Compose this with sub.But this is shallow composition nd need to traverse "chain"
         * to get bound value.
         */
        Substitution* operator +(Substitution& sub);
        Mapping getMapping(){return mappping;}
        ~Substitution();
    };
    
    #define EMPTY_SUB Substitution::emptySub
} // namespace Ares
#endif 
