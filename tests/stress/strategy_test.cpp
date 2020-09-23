#include "static.hh"
#include "strategy/random.hh"
#include "strategy/montecarlo.hh"
ares::Cfg ares::cfg;
void takeIn(const ares::State* state,std::vector<ares::uMoves>*  moves,int& m);
void takeIn(const ares::State* state,ares::Moves*  moves, ares::Moves* moves1,const ares::role_sptr& r0,const ares::role_sptr& r1, int& m0,int& m1);
using namespace ares;
int main(int argc, char const *argv[])
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
    Body::mempool = ClauseBody::mempool = &mempool;
    SuffixRenamer::setPool(mempool.getCache());

    //Create Ares
    auto& strategy = Registrar::get(cfg.strategy.c_str());
    Reasoner& reasoner(Reasoner::create(GdlParser::create(mempool.getCache()), Prover::create(), *mempool.getCache()));
    Ares& ares( Ares::create(strategy,reasoner));

    //Start game, this is done after the start message has been recieved
    //Create the knowledge base
    Game* kb(new Game());
   
    //Parse the gdl
    ares.parser.parse(kb,cfg.gdlFile);
    Match match;
    match.game = kb; 
    match.matchId = "test.0"; 
    match.strtClck = 2; 
    match.plyClck = 2; 
    match.role = kb->getRoles()[0];
    cfg.delta_sec = 200;
    std::cout << "ares playing : "<<Namer::name(match.role->get_name()) << "\n";
    ares.startMatch(match,Namer::name(match.role->get_name()));

    const role_sptr& r0 = ares->roles()[0];
    const role_sptr& r1 = ares->roles()[1];

    std::cout << "-----Roles------\n\n";
    std::cout << *r0 << std::endl;
    std::cout << *r1 << std::endl;
    std::cout << "-----Roles------\n\n";
    std::cout << cfg << "\n";
    srand(time(NULL));

    //Get the initial state
    const State& init = ares->init();
    uint sims = 0;

    auto c = std::chrono::high_resolution_clock();
    auto begin = c.now();
    uint total_steps = 0;
    Moves* prev=nullptr;
    uint seq = 0;
    while (sims < cfg.simulations)
    {
        const State* state = &init;
        ushort steps=0;
        while (steps < cfg.steps)
        {
            if( ares->terminal(*state) ){
                auto reward_0 = ares->reward(*r0, state);
                auto reward_1 = ares->reward(*r1, state);
                std::cout << "Rewards for " << *r0<< " , " << reward_0 << "\n";
                std::cout << "Rewards for " << *r1<< " , " << reward_1 << "\n";
                break;
            }


            Moves* moves = ares->moves(*state, *r0,false);
            Moves* moves1 = ares->moves(*state, *r1,false);
            
            auto amove = ares.makeMove(++seq,prev);
            auto j = rand() % moves->size();
            auto i = rand() % moves1->size();

            std::cout << "selecting " << (*moves1)[i]->to_string() << " randomly\n";
            assert( amove.first and (amove.second == seq) );
            prev = new Moves{amove.first , (*moves1)[i]};
            
            State* nxt = ares->next(*state, *prev);

            delete moves;
            delete moves1;
            
            if( state != &init ) delete state;
            state = nxt;
            // return 0;
            steps++;
        }
        std::cout << "Steps : " << steps << "\n";
        sims++;
        total_steps+=steps;
    }
    auto end = c.now();
    auto dur = std::chrono::duration_cast<std::chrono::microseconds>(end-begin);
    log("[StrategyTest] Total time of program execution : ") << dur.count() <<" microseconds\n";
    log("[StrategyTest] Average time per step: ") << (dur.count()/total_steps) <<" microseconds\n";
    log("[StrategyTest] Succesfull.");
    return 0;
}