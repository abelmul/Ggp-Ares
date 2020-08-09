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

            gdlFile = pt.get<std::string>("gdl");

            std::ifstream f(gdlFile);
            gdl = std::string((std::istreambuf_iterator<char>(f)), std::istreambuf_iterator<char>());
        }
        friend std::ostream& operator<<(std::ostream& os, const Cfg& cfg);
        std::string str()const{
            // std::string s = boost::format().str();
            auto pbf = boost::format("%|=20| : %|-5|\n") % "proverThreads" % proverThreads;
            auto dqf = boost::format("%|=20| : %|-5|\n") % "deletionQueueSize" % deletionQueueSize;
            auto dpf = boost::format("%|=20| : %|-5|\n") % "deletionPeriod" % deletionPeriod;
            auto gf = boost::format("%|=20| : %|-5|\n") % "gdl" % gdlFile;
            auto sf = boost::format("%|=20| : %|-5|\n") % "simulaions" % simulaions;
            auto stf = boost::format("%|=20| : %|-5|\n") % "steps" % steps;
            auto df = boost::format("%|=20| : %|-5|\n") % "debug" % debug;
            auto rf = boost::format("%|=20| : %|-5|\n") % "random   " % random ;
            return  pbf.str() + dqf.str() + dpf.str() + gf.str()+ sf.str()+ stf.str()+ df.str()+ rf.str();
        }
        std::string gdl;
        std::string gdlFile;
        uint parserThreads;
        uint proverThreads;
        uint deletionPeriod;
        uint deletionQueueSize;
        uint stTerms;
        uint clauses;
        std::vector<std::pair<uint,uint>> arities;
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