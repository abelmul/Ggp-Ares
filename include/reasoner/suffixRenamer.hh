#ifndef SUFFIX_RENAMER_HH
#define SUFFIX_RENAMER_HH

#include "reasoner/substitution.hh"
#include <mutex>
#include <string.h>

namespace Ares
{
    class ExpressionPool;
    /**
     * Renames variables by adding a unique suffix to them.
     */
    class SuffixRenamer : Substitution
    {
    private:
        static std::mutex smutex;
        static uint suffix;                     //Universally unique 

        static ExpressionPool* pool;
        uint current_suffix;                    //Save it for the current renaming

    public:
        SuffixRenamer(){ 
            std::lock_guard<std::mutex> lk(smutex);
            current_suffix = suffix++;
        }

        /**
         * Protect against accidental copying, pass by value, ...
         */
        SuffixRenamer(const SuffixRenamer& s) = delete;
        SuffixRenamer& operator = (const SuffixRenamer& other) = delete;

        bool bind(const Variable* x, const Term* t){ return true;}
        // const Term* Substitution::get(const Variable* x) const 
        // const Term* Substitution::operator[] (const Variable* x) const
        // bool Substitution::isBound(const Variable* x) const 

        //rename x
        const Term* get(const Variable* x) const ;
        //rename x        
        const Term* operator[](const Variable* x) const {  return get(x); }

        bool isRenaming() const { return true;}
        /**
         * Any variable can be renamed
         */ 
        bool isBound(const Variable* x) const { return true;}
    };
} // namespace Ares

#endif