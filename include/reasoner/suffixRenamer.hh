#ifndef SUFFIX_RENAMER_HH
#define SUFFIX_RENAMER_HH

#include "reasoner/substitution.hh"
#include <mutex>
#include <string.h>

namespace ares
{
    class MemCache;
    /**
     * Renames variables by adding a unique suffix to them.
     */
    class SuffixRenamer : public Substitution
    {
    private:
        static MemCache* pool;
        ushort suffix = 1;                    //Save it for the current renaming

    public:
        SuffixRenamer(){ }
        SuffixRenamer(const SuffixRenamer& sr):Substitution(){ suffix=sr.suffix;}
        SuffixRenamer(const SuffixRenamer&& s) { suffix = s.suffix;}

        void setSuffix(uint suff ) { suffix = suff;}
        ushort getNxtSuffix() const{ return suffix+1;}
        static void setPool(MemCache* p) {pool = p;}
        /**
         * delete assignments.
         */
        SuffixRenamer& operator = (const SuffixRenamer& other) = delete;
        SuffixRenamer& operator = (const SuffixRenamer&& other) = delete;

        static Substitution emptySub;

        virtual bool bind(const Variable*,const cnst_term_sptr&){return true;}

        //To get the immediate mapping, without traversing the chain.
        virtual const cnst_term_sptr get(const Variable*) const ;
        //Overload the indexing operator, to get the underlying exact mapping        
        virtual const cnst_term_sptr operator[]  (const Variable* x) const{ return get(x);} 

        virtual bool isRenaming() const { return true;}
        /**
         * Check if this variable is bound
         */ 
        virtual bool isBound(const Variable*) const {return true;}
        
        virtual ~SuffixRenamer(){}
    };
} // namespace Ares

#endif