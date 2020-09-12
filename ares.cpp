#include "static.hh"
#include "utils/utils/httpHandler.hh"
#include "strategy/random.hh"

ares::Cfg ares::cfg;

int main(int argc, char const *argv[])
{
    srand(time(NULL));
    std::cout.setf(std::ios::unitbuf);
    using namespace ares;
    //Read in the configuration file
    cfg = Cfg("./ares.cfg.json");

    //Create the memory pool
    std::vector<std::pair<arity_t, uint>> arities = {make_pair(1,32768),make_pair(2,32768),make_pair(3,32768),make_pair(4,32768),make_pair(5,32768),make_pair(6,32768),make_pair(7,32768),make_pair(8,32768),make_pair(9,4096),make_pair(10,32768),make_pair(11,4096),make_pair(12,4096)};
    MemoryPool& mempool = MemoryPool::create(131072,262144,arities);
    Ares::setMem(&mempool);

    //Get the singleton prover
    Prover& prover(Prover::create());
    ClauseCB::prover = &prover;

    //Setup some static elements
    Body::mempool = ClauseBody::mempool = &mempool;
    SuffixRenamer::setPool(mempool.getCache());

    //Create the parser
    GdlParser& p = GdlParser::create(cfg.parserThreads,mempool.getCache());

    //Create a reasoner over a game
    Reasoner& reasoner(Reasoner::create(p, prover,*mempool.getCache()));
    Strategy::setReasoner(&reasoner);
    
    //Create Ares
    Ares& ares( Ares::create(reasoner, Registrar::get(cfg.strategy.c_str()), p));

    //Start the server!
    HttpHandler(ares,std::string("http://192.168.0.50:8080"))
    .wait();
    return 0;
}   

