#include "static.hh"
#include "strategy/montecarlo.hh"
#include "strategy/random.hh"
ares::Cfg ares::cfg;
using namespace ares;
void simulate(Ares& ares);
int main()
{
    srand(time(NULL));
    std::cout.setf(std::ios::unitbuf);

    // Read in the configuration file
    cfg = Cfg("./ares.cfg.json");
    log("[Ares]\n" + cfg.str());
    // Set the global memory pool
    Ares::setMem(&mempool);

    // Setup some static elements
    ClauseCB::prover = &Prover::create();
    Body::mempool = &mempool;

    // Create Ares
    auto& strategy = Registrar::get(cfg.strategy.c_str());
    Reasoner& reasoner(Reasoner::create(GdlParser::create(mempool.getCache()),
                                        Prover::create(), *mempool.getCache()));
    Ares& ares(Ares::create(strategy, reasoner));
    uint sims = 0;
    while (sims++ < cfg.simulations) {
        log("[StrategyTest]") << "Simulation #" << sims << "\n";
        simulate(ares);
    }
    return 0;
}

void simulate(Ares& ares)
{
    ares.abortMatch("test.0");
    // Start game, this is done after the start message has been recieved
    // Create the knowledge base
    Game* kb(new Game());

    // Parse the gdl
    std::string gdl("tests/stress/selectedGames/ticTacToe.kif");
    ares.parser.parse(kb, gdl);
    Match match;
    match.game = kb;
    match.matchId = "test.0";
    match.strtClck = 5;
    match.plyClck = 5;
    cfg.delta_milli = 200;
    ares.startMatch(match, "xplayer");
    std::cout << "ares playing : "
              << "xplayer"
              << "\n";

    const Role* r0 = ares->roles()[0];
    const Role* r1 = ares->roles()[1];
    srand(time(NULL));

    // Get the initial state
    const State& init = ares->init();

    auto c = std::chrono::high_resolution_clock();
    auto begin = c.now();
    uint total_steps = 0;
    Moves* prev = nullptr;
    uint seq = 0;
    const State* state = &init;
    ushort steps = 0;
    while (steps < cfg.steps) {
        if (ares->terminal(*state)) {
            auto reward_0 = ares->reward(*r0, state);
            auto reward_1 = ares->reward(*r1, state);
            std::cout << "Rewards for " << *r0 << " , " << reward_0 << "\n";
            std::cout << "Rewards for " << *r1 << " , " << reward_1 << "\n";
            break;
        }

        Moves* moves = ares->moves(*state, *r0, false);
        Moves* moves1 = ares->moves(*state, *r1, false);

        auto amove = ares.makeMove(++seq, prev);
        auto i = rand() % moves1->size();

        assert(amove.first and (amove.second == seq));
        prev = new Moves{amove.first, (*moves1)[i]};

        State* nxt = ares->next(*state, *prev);

        delete moves;
        delete moves1;

        if (state != &init)
            delete state;
        state = nxt;
        // return 0;
        steps++;
    }
    std::cout << "Steps : " << steps << "\n";
    total_steps += steps;
    auto end = c.now();
    auto dur =
        std::chrono::duration_cast<std::chrono::microseconds>(end - begin);
    log("[StrategyTest] Total time of program execution : ")
        << dur.count() << " microseconds\n";
    log("[StrategyTest] Average time per step: ")
        << (dur.count() / total_steps) << " microseconds\n";
    log("[StrategyTest] Succesfull.\n");
}
