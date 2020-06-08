#ifndef VARRENAMER_HH
#define VARRENAMER_HH

#include "reasoner/substitution.hh"
#include "utils/gdl/variable.hh"
#include <mutex>
#include <cmath>
#include <sstream>

namespace Ares
{
    class VarRenamer
    {
    public:
        VarRenamer(const VarRenamer& vr) = delete;
        VarRenamer& operator =(const VarRenamer& vr) = delete;
        
        VarRenamer(/* args */){};
        virtual Substitution* rename(Substitution& theta) = 0;
        ~VarRenamer();
    };
} // namespace Ares

#endif