#include "ares.hh"
#include "static.hh"
#include "strategy/random.hh"
#include "strategy/montecarlo.hh"
#include <filesystem>
#include <iostream>
#include <random>
ares::Cfg ares::cfg;

using namespace ares;

#define GAMES_DIR "tests/stress/selectedGames"
#define PERF_MD   "performance.md"

std::ofstream perf(PERF_MD,std::ios::app);

void profile(std::string gdl,ares::Ares& ares);
uint doSimulation(ares::Ares& ares);

// void doSimulation();
/**
 * This test is inteded to do game simulations and reveal either conccurency problems
 * or memory problems. As well as profile different implementaitons.
 */
int main()
{
    srand(time(NULL));
    std::cout.setf(std::ios::unitbuf);
    
    //Read in the configuration file
    cfg = Cfg("./ares.cfg.json");
    log("[Ares]\n" + cfg.str());
    //Set the global memory pool
    Ares::setMem(&mempool);

    //Setup some static elements
    ClauseCB::prover = &Prover::create();
    Body::mempool = &mempool;

    //Create Ares
    Reasoner& reasoner(Reasoner::create(GdlParser::create(mempool.getCache()), Prover::create(), *mempool.getCache()));
    Ares& ares( Ares::create(Registrar::get(cfg.strategy.c_str()),reasoner));


    perf << "\n\n## Id " << cfg.impNo <<"\n\n";
    perf << "|Game  | Iter1   	| Iter2  | Iter3  |   Iter4  |\n";
    perf << "|-     |-          |-      |-      |-        | \n";

    std::vector<std::string> games{"tests/stress/selectedGames/connect4.kif","tests/stress/selectedGames/ticTacToe.kif",
    "tests/stress/selectedGames/bomberman2p.kif","tests/stress/selectedGames/knightsTour.kif",
    "tests/stress/selectedGames/cephalopodMicro.kif","tests/stress/selectedGames/chess.kif"};
    perf.setf(std::ios::unitbuf);

    for (size_t i = 0; i < games.size(); i++)
        profile(games[i],ares);
    return 0;
}

void profile(std::string gdl,ares::Ares& ares){
    namespace fs = std::filesystem;
    //Start game, this is done after the start message has been recieved
    //Create the knowledge base
    log("[Simulator]") << "Using file " << gdl << "\n";
    log("[Simulator]") << "Doing random simulations for 20 seconds.\n";
    Game* kb(new Game());
    //Parse the gdl
    ares.parser.parse(kb,gdl);
    ares->reset(nullptr);
    ares->reset(kb);
    srand(time(NULL));

    size_t start_pos = gdl.find(".kif");
    gdl.replace(start_pos, 4, "");
    perf <<"|" << fs::path(gdl).filename() << "|";
    for (size_t i = 1; i <= 4; i++)
    {
        log("[Simulator]") << "Iteration " << i <<"\n";
        perf << doSimulation(ares) << "|";
    }
    perf <<"\n";
    
}
uint doSimulation(ares::Ares& ares){
    uint total_steps=0;
    atomic_uint32_t sims=0,sims_end=0;
    const State& init = ares->init();
    std::atomic_bool done = false;
    std::thread th([&]{
        std::this_thread::sleep_for(std::chrono::seconds(20));
        done = true;
        sims_end = sims.load();
    });
    // auto c = std::chrono::high_resolution_clock();
    // auto begin = c.now();
    while (not done)
    {
        const State* state = &init;
        ushort steps=0;

        while ((not done) and ( not ares->terminal(*state)) )
        {   
            Action* action;
            action = ares->randAction(*state);
            State* nxt = ares->next(*state, *action);
            delete action;
            
            if( state != &init ) delete state;
            state = nxt;
            // return 0;
            steps++;
        }
        sims++;
        // auto end = c.now();
        // std::cout << "steps " << steps << "\n";
        // auto dur = std::chrono::duration_cast<std::chrono::seconds>(end-begin);
        // std::cout << "Time for a simulation "<< sims <<" " << dur.count() << "seconds.\n";
        if( state != &init ) delete state;
        total_steps+=steps;
    }
    th.join();
    return sims_end;
}