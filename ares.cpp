#include "ares.hh"

int main(int argc, char const *argv[])
{
    using namespace Ares;
    using boost::property_tree::ptree;

    std::ifstream cfg("./ares.cfg.json");

    ptree pt;
    read_json(cfg, pt);
    std::string gdlF = pt.get<std::string>("gdl");
    std::ifstream f(gdlF);
    std::string gdl((std::istreambuf_iterator<char>(f)),
                 std::istreambuf_iterator<char>());
    GdlParser* p = GdlParser::getParser(1);

    p->parse(new Game(),gdl);
    return 0;
}
