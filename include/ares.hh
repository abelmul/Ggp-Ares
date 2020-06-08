#ifndef ARES_HH
#define ARES_HH

#include "utils/gdl/variable.hh"
#include "utils/gdl/function.hh"
#include "utils/gdl/constant.hh"
#include "reasoner/unifier.hh"
#include "utils/gdl/gdlParser/gdlParser.hh"
#include "utils/game/game.hh"
#include <iostream>
#include <fstream>
#include <boost/property_tree/ptree.hpp>
#include <boost/property_tree/json_parser.hpp>
#include <boost/foreach.hpp>

namespace Ares
{
    //Initialize static nameHasher
    CharpHasher Term::nameHasher;

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
    
} // namespace Ares

#endif