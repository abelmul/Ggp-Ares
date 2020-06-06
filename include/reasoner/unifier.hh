#ifndef UNIFIER_HH
#define UNIFIER_HH

#include "utils/gdl/literal.hh"
#include "utils/gdl/term.hh"
#include "utils/gdl/variable.hh"
#include "utils/gdl/constant.hh"
#include "utils/gdl/function.hh"

#include <string.h>

namespace Ares
{
    class Unifier
    {
    private:
        Unifier(/* args */){};
    public:
        static bool unifyPredicate(Literal& l1, Literal& l2,Substitution& sub);
        static bool unifyTerm(Term& t1, Term& t2,Substitution& sub);
        ~Unifier(){}
    };
} // namespace Ares

#endif