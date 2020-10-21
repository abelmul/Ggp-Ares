#include "static.hh"
#include "strategy/montecarlo.hh"
#include "strategy/random.hh"
#include "utils/utils/httpHandler.hh"
using namespace ares;
ares::Cfg ares::cfg;

int main()
{
    srand(time(NULL));
    std::cout.setf(std::ios::unitbuf);

    // Read in the configuration file
    cfg = Cfg("./ares.cfg.json");
    log("[Ares] configurations...\n" + cfg.str());

    // Set the global memory pool
    Ares::setMem(&mempool);

    // Setup some static elements
    ClauseCB::prover = &Prover::create();
    Body::mempool = &mempool;

    // Create Ares
    Reasoner& reasoner(Reasoner::create(GdlParser::create(mempool.getCache()),
                                        Prover::create(), *mempool.getCache()));
    Ares& ares(Ares::create(Registrar::get(cfg.strategy.c_str()), reasoner));

    // Start the server!
    HttpHandler(ares, cfg.url).wait();
    return 0;
}
