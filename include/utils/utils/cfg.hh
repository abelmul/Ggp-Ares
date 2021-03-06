#ifndef UTILS_HH
#define UTILS_HH
#include "utils/utils/color.hh"

#include <boost/foreach.hpp>
#include <boost/property_tree/json_parser.hpp>
#include <boost/property_tree/ptree.hpp>
#include <fstream>
#include <initializer_list>
#include <iostream>
namespace ares
{
    typedef bool Determinism;

    inline std::ostream& log(const std::string& msg)
    {
        using namespace rang;
        using namespace std;
        cout << style::bold << fg::green << msg << " " << style::reset;
        return cout;
    }

    inline std::ostream& logerr(const std::string& msg)
    {
        using namespace rang;
        using namespace std;
        cout << style::bold << fg::red << msg << " " << style::reset;
        return cout;
    }

    struct Cfg {
        Cfg() {}
        Cfg(std::string cfg_f)
        {
            using boost::property_tree::ptree;

            std::ifstream cfg_s(cfg_f);

            ptree pt;
            read_json(cfg_s, pt);

            url = pt.get<std::string>("url");
            stateDumpF = pt.get<std::string>("stateDumpF");
            strategy = pt.get<std::string>("strategy");
            mct_threads = pt.get<uint>("mct_threads");
            debug = pt.get<bool>("debug");
            random = pt.get<bool>("random");
            simulations = pt.get<uint>("simulations");
            impNo = pt.get<uint>("impNo");
            steps = pt.get<uint>("steps");
            ansSample = pt.get<ushort>("ansSample");
            delta_milli = pt.get<ushort>("delta_milli");
            uct_c = pt.get<ushort>("uct_c");
            bucket = pt.get<uint>("bucket");

            // jobQueue          = pt.get<uint>("jobQueue");
            if (not pt.get<bool>("file"))
                return;

            gdlFile = pt.get<std::string>("gdl");

            std::ifstream f(gdlFile);
            gdl = std::string((std::istreambuf_iterator<char>(f)),
                              std::istreambuf_iterator<char>());
        }
        friend std::ostream& operator<<(std::ostream& os, const Cfg& cfg);
        std::string str() const
        {
            // std::string s = boost::format().str();
            auto strf =
                boost::format("%|-20| : %|-5|\n") % "Strategy" % strategy;
            auto pbf =
                boost::format("%|-20| : %|-5|\n") % "mct_threads" % mct_threads;
            auto gf = boost::format("%|-20| : %|-5|\n") % "gdl" % gdlFile;
            auto urlf = boost::format("%|-20| : %|-5|\n") % "url" % url;
            auto sf =
                boost::format("%|-20| : %|-5|\n") % "simulations" % simulations;
            auto stf = boost::format("%|-20| : %|-5|\n") % "steps" % steps;
            auto dltf = boost::format("%|-20| : %|-5|\n") %
                        "delta milli secods" % delta_milli;
            auto ansf =
                boost::format("%|-20| : %|-5|\n") % "Answer sample" % ansSample;
            auto uctf = boost::format("%|-20| : %|-5|\n") % "uct C" % uct_c;
            auto bf = boost::format("%|-20| : %|-5|\n") % "Buckets" % bucket;
            auto df = boost::format("%|-20| : %|-5|\n") % "debug" % debug;
            auto rf = boost::format("%|-20| : %|-5|\n") % "random   " % random;
            return strf.str() + urlf.str() + pbf.str() + gf.str() + sf.str() +
                   stf.str() + df.str() + rf.str() + bf.str() + dltf.str() +
                   ansf.str() + ansf.str() + uctf.str();
        }
        std::string strategy;
        std::string gdl;
        std::string url;
        std::string stateDumpF;
        std::string gdlFile;
        uint mct_threads;
        uint stTerms;
        uint clauses;
        ushort ansSample;
        ushort uct_c;
        ushort delta_milli;
        std::vector<std::pair<uint, uint>> arities;
        // uint jobQueue;
        uint simulations;
        uint steps;
        uint bucket;
        bool debug;
        bool random;
        uint impNo;
    };
    extern Cfg cfg;

    inline void debug()
    {
        std::cout << "\n";
        fflush(NULL);
    }
    template <class T, class... Types>
    void debug(T s1, Types... args)
    {
        if (not cfg.debug)
            return;

        std::cout << s1 << " ";
        debug(args...);
    }
}  // namespace ares

#endif