#ifndef ARES_HH
#define ARES_HH

#include "utils/gdl/variable.hh"
#include "utils/gdl/function.hh"
#include "utils/gdl/constant.hh"
#include "reasoner/unifier.hh"
#include <iostream>

namespace Ares
{
    bool DEBUG=false;

    class Ares
    {
    private:
        /* data */
    public:
        Ares(/* args */);
        ~Ares();
    };
    
    Ares::Ares(/* args */)
    {
    }
    
    Ares::~Ares()
    {
    }
    #define DEBUG_ARES
} // namespace Ares

#endif