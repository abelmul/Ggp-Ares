#ifndef SUBST_HH
#define SUBST_HH

#include <unordered_map>
#include <functional>
#include <string>
#include "utils/utils/hashing.hh"

namespace ares
{

    class Variable;
    class Term;
    
    typedef std::unordered_map<cnst_var_sptr,cnst_term_sptr,SpVarHasher,SpVarEqual> Mapping;

    class Substitution
    {

    private:
        Mapping mapping;

    public:
        /**
         * Protect against accidental copying, pass by value, ...
         */
        Substitution& operator = (const Substitution& other) = delete;
        Substitution& operator = (const Substitution&& other) = delete;
        Substitution(const Substitution&& s) = delete;
        
        Substitution(const Substitution& s){
            this->mapping.insert(s.mapping.begin(),s.mapping.end());
        }

        Substitution(){}
        static Substitution emptySub;

        virtual bool bind(cnst_var_sptr&,const cnst_term_sptr& t);

        //To get the immediate mapping, without traversing the chain.
        virtual const cnst_term_sptr get(cnst_var_sptr&) const ;
        //Overload the indexing operator, to get the underlying exact mapping        
        virtual const cnst_term_sptr operator[]  (cnst_var_sptr&) const ;

        virtual bool isRenaming() const { return false;}
        /**
         * Check if this variable is bound
         */ 
        virtual bool isBound(cnst_var_sptr&) const;
        /**
         * Compose this with sub.But this is shallow composition nd need to traverse "chain"
         * to get bound value.
         */
        bool isEmpty() const { return mapping.size() == 0;}
        Substitution* operator +(Substitution& sub);
        Mapping getMapping(){return mapping;}
        std::string to_string() const ;

        virtual ~Substitution(){}
    };
    
    #define EMPTY_SUB Substitution::emptySub
} // namespace ares
#endif 
