#include "ares.hh"
#include <chrono> 
using namespace std::chrono;

int main(int argc, char const *argv[])
{
    using namespace Ares;
    using boost::property_tree::ptree;

    std::ifstream cfg("./ares.cfg.json");

    ptree pt;
    read_json(cfg, pt);
    std::string gdlF = pt.get<std::string>("gdl");
    uint nThreads = pt.get<uint>("parser_threads");
    std::ifstream f(gdlF);
    std::string gdl((std::istreambuf_iterator<char>(f)),
                 std::istreambuf_iterator<char>());
    
    GdlParser* p = GdlParser::getParser(nThreads);
    
    auto start = high_resolution_clock::now();
    p->parse(new Game(),gdl);
    auto stop = high_resolution_clock::now(); 
    cout << duration_cast<milliseconds>(stop - start).count() << endl;

    return 0;
}
