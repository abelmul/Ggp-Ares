#ifndef SUFFIX_RENAMER_HH
#define SUFFIX_RENAMER_HH

#include "reasoner/substitution.hh"
#include <mutex>
#include <string.h>
#include <atomic>
namespace ares
{
    class MemCache;
    /**
     * Renames variables by adding a unique suffix to them.
     */
    class SuffixRenamer : public Substitution
    {
    private:            
        const ushort suffix;                    

    public:
        SuffixRenamer(ushort s):suffix(s+1){}
        /**
         * delete copy/move constructor/assignment.
         */
        SuffixRenamer(const SuffixRenamer&)= delete;
        SuffixRenamer(const SuffixRenamer&&) = delete;
        SuffixRenamer& operator = (const SuffixRenamer& other) = delete;
        SuffixRenamer& operator = (const SuffixRenamer&& other) = delete;

        static Substitution emptySub;

        virtual bool bind(const Variable*,const Term*){return true;}
        
        ushort gets() { return suffix;}
        //To get the immediate mapping, without traversing the chain.
        virtual const Term* get(const Variable*) const ;
        //Overload the indexing operator, to get the underlying exact mapping        
        virtual const Term* operator[]  (const Variable* x) const{ return get(x);} 

        virtual bool isRenaming() const { return true;}
        /**
         * Check if this variable is bound
         */ 
        virtual bool isBound(const Variable*) const {return true;}
        
        virtual ~SuffixRenamer(){}
    };
} // namespace Ares

#endif