#ifndef SUFFIX_RENAMER_HH
#define SUFFIX_RENAMER_HH

#include "reasoner/substitution.hh"
#include <mutex>
#include <string.h>

namespace ares
{
    class ExpressionPool;
    /**
     * Renames variables by adding a unique suffix to them.
     */
    class SuffixRenamer : public Substitution
    {
    private:
        static ExpressionPool* pool;
        ushort suffix = 1;                    //Save it for the current renaming

    public:
        SuffixRenamer(){ }
        SuffixRenamer(const SuffixRenamer& sr):Substitution(){ suffix=sr.suffix;}
        void setSuffix(uint suff ) { suffix = suff;}
        ushort getNxtSuffix() { return suffix+1;}
        static void setPool(ExpressionPool* p) {pool = p;}
        /**
         * Protect against accidental copying, pass by value, ...
         */
        SuffixRenamer(const SuffixRenamer&& s) = delete;
        SuffixRenamer& operator = (const SuffixRenamer& other) = delete;
        SuffixRenamer& operator = (const SuffixRenamer&& other) = delete;

        static Substitution emptySub;

        virtual bool bind(const cnst_var_sptr&,const cnst_term_sptr&){return true;}

        //To get the immediate mapping, without traversing the chain.
        virtual const cnst_term_sptr get(const cnst_var_sptr&) const ;
        //Overload the indexing operator, to get the underlying exact mapping        
        virtual const cnst_term_sptr operator[]  (const cnst_var_sptr& x) const{ return get(x);} 

        virtual bool isRenaming() const { return true;}
        /**
         * Check if this variable is bound
         */ 
        virtual bool isBound(const cnst_var_sptr&) const {return true;}
        
        virtual ~SuffixRenamer(){}
    };
} // namespace Ares

#endif