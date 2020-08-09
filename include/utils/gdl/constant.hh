#ifndef CONST_HH
#define CONST_HH

#include "utils/gdl/term.hh"
#include <iostream>

namespace ares
{
    class Constant: public Term
    {
    friend class MemCache;
    friend class ExpressionPoolTest;
    Constant(ushort name):Term(name,CONST){}
    public:
        /**
         * Deleting a constant does nothing.
         * The Memory pool will free the malloc'd memory.
         */
        ~Constant(){ }
        void operator delete(void*){}
        virtual const cnst_term_sptr operator ()(const Substitution &,VarSet&) const {
            return cnst_term_sptr(this,[](const Term*){});
        }
        virtual bool is_ground() const {
            return true;
        }
        virtual std::string to_string() const {
            return Namer::name(name);
        }
        virtual std::size_t hash() const {
            return std::hash<ushort>()(name);
        }
    };
    
} // namespace ares

#endif