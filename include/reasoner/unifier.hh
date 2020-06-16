#ifndef UNIFIER_HH
#define UNIFIER_HH

#include "utils/gdl/term.hh"
#include "utils/gdl/literal.hh"
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
        static bool unifyPredicate(const Literal& l1,const  Literal& l2,Substitution& sub);
        static bool unifyTerm(const Term& t1,const  Term& t2,Substitution& sub);
        ~Unifier(){}
    };
} // namespace Ares

#endif