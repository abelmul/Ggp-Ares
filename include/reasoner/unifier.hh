#ifndef UNIFIER_HH
#define UNIFIER_HH

#include "utils/gdl/gdl.hh"

#include <string.h>

namespace ares
{
    class Unifier
    {
    private:
        Unifier(/* args */){};
    public:
        static bool unifyAtom(const Atom& l1,const Atom& l2,Substitution& sub);
        static bool unifyTerm(const Term* t1,const Term* t2,Substitution& sub);
        ~Unifier(){}
    };
} // namespace ares

#endif