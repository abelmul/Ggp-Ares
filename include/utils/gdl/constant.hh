#ifndef CONST_HH
#define CONST_HH

#include "utils/gdl/term.hh"
#include <iostream>

namespace ares
{
    class Constant: public Term
    {
    friend class ExpressionPool;
    friend class ExpressionPoolTest;
    Constant(const char* name,cnst_const_sptr* _this):Term(name,(cnst_term_sptr*)_this,CONST){}
    
    public:
        /**
         * Deleting a constant does nothing.
         * The Memory pool will free the malloc'd memory.
         */
        ~Constant(){ }
        void operator delete(void* p){}
        virtual const cnst_term_sptr operator ()(const Substitution &sub,VarSet& vStack) const {
            return *_this;
        }
        virtual bool is_ground() const {
            return true;
        }
        virtual std::string to_string() const {
            return std::string(name);
        }
        virtual std::size_t hash() const {
            return nameHasher(name);
        }
    };
    
} // namespace ares

#endif