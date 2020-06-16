#ifndef ARES_HH
#define ARES_HH

#include "reasoner/suffixRenamer.hh"
#include "utils/gdl/variable.hh"
#include "utils/gdl/function.hh"
#include "utils/gdl/constant.hh"
#include "reasoner/reasoner.hh"
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

    struct Cfg
    {
        Cfg(std::string cfg_f){
            using boost::property_tree::ptree;

            std::ifstream cfg_s(cfg_f);

            ptree pt;
            read_json(cfg_s, pt);
            
            parserThreads = pt.get<uint>("parser_threads");
            proverThreads = pt.get<uint>("prover_threads");
            negThreads    = pt.get<uint>("neg_threads");
            if( not pt.get<bool>("file") ) return;

            std::string gdlF = pt.get<std::string>("gdl");

            std::ifstream f(gdlF);
            gdl = string((std::istreambuf_iterator<char>(f)), std::istreambuf_iterator<char>());
        }

        std::string gdl;
        uint parserThreads;
        uint proverThreads;
        uint negThreads;
    };
    
} // namespace Ares

#endif  