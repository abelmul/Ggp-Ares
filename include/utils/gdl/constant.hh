#ifndef CONST_HH
#define CONST_HH

#include "utils/gdl/term.hh"

#include <iostream>

namespace ares
{
    class Constant : public Term
    {
        friend class MemCache;
        friend class ExpressionPoolTest;
        Constant(ushort name) : Term(name, CONST) {}
        /**
         * Deleting a constant does nothing.
         * The Memory pool will free the malloc'd memory.
         */
        ~Constant() {}

     public:
        virtual const Term* operator()(const Substitution&, VarSet&) const
        {
            return this;
        }
        virtual bool is_ground() const { return true; }
        virtual std::string to_string() const { return Namer::name(name); }
        virtual std::size_t hash() const { return std::hash<ushort>()(name); }
        virtual std::size_t hash(VarRenaming&, ushort&) const { return hash(); }

        virtual bool equals(const Term& t, VarRenaming&) const
        {
            return (t.get_type() == CONST) and t.get_name() == name;
        }
    };

}  // namespace ares

#endif