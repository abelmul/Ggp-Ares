#ifndef UTILS_HH
#define UTILS_HH
#include <iostream>
#include <fstream>
#include <boost/property_tree/ptree.hpp>
#include <boost/property_tree/json_parser.hpp>
#include <boost/foreach.hpp>
#include  <initializer_list>

namespace ares
{
    struct Cfg
    {
        Cfg(){}
        Cfg(std::string cfg_f){
            using boost::property_tree::ptree;

            std::ifstream cfg_s(cfg_f);

            ptree pt;
            read_json(cfg_s, pt);
            
            parserThreads     = pt.get<uint>("parser_threads");
            proverThreads     = pt.get<uint>("prover_threads");
            debug             = pt.get<bool>("debug");
            random             = pt.get<bool>("random");
            simulaions        = pt.get<uint>("simulaions");
            steps        = pt.get<uint>("steps");
            deletionPeriod    = pt.get<uint>("deletionPeriod");
            deletionQueueSize = pt.get<uint>("deletionQueueSize");
            // jobQueue          = pt.get<uint>("jobQueue");
            if( not pt.get<bool>("file") ) return;

            std::string gdlF = pt.get<std::string>("gdl");

            std::ifstream f(gdlF);
            gdl = std::string((std::istreambuf_iterator<char>(f)), std::istreambuf_iterator<char>());
        }

        std::string gdl;
        uint parserThreads;
        uint proverThreads;
        uint deletionPeriod;
        uint deletionQueueSize;
        // uint jobQueue;
        uint simulaions;
        uint steps;
        bool debug;
        bool random;
    };
    extern Cfg cfg;

    inline void debug(){ 
        std::cout << "\n";
        fflush(NULL);
    }
    template<class T, class... Types>
    void debug(T s1, Types... args){
        if( not cfg.debug ) return;

        std::cout << s1 << " ";
        debug(args...);
    }
} // namespace ares

#endif