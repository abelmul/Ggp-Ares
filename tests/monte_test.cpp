#include "static.hh"
#include "strategy/random.hh"
#include "strategy/montecarlo.hh"
ares::Cfg ares::cfg;
void takeIn(const ares::State* state,std::vector<ares::Reasoner::unique_moves>*  moves,int& m);
void takeIn(const ares::State* state,ares::Moves*  moves, ares::Moves* moves1,const ares::role_sptr& r0,const ares::role_sptr& r1, int& m0,int& m1);
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

    //Start game, this is done after the start message has been recieved
    //Create the knowledge base
    Game* kb(new Game());
   
    //Parse the gdl
    p.parse(kb,cfg.gdlFile);
    Match match;
    match.game = kb; 
    match.matchId = "test.0"; 
    match.strtClck = 125; 
    match.plyClck = 125; 
    match.role = kb->getRoles()[0];
    std::cout << "ares playing : "<<Namer::name(match.role->get_name()) << "\n";
    ares.startMatch(match,Namer::name(match.role->get_name()));

    const role_sptr& r0 = reasoner.roles()[0];
    const role_sptr& r1 = reasoner.roles()[1];

    std::cout << "-----Roles------\n\n";
    std::cout << *r0 << std::endl;
    std::cout << *r1 << std::endl;
    std::cout << "-----Roles------\n\n";
    std::cout << cfg << "\n";
    srand(time(NULL));
    visualizer viz;

    //Get the initial state
    const State& init = reasoner.init();
    uint sims = 0;

    auto c = std::chrono::high_resolution_clock();
    auto begin = c.now();
    uint total_steps = 0;
    Moves* prev=nullptr;
    uint seq = 0;
    while (sims < cfg.simulaions)
    {
        const State* state = &init;
        ushort steps=0;
        while (steps < cfg.steps)
        {
            if( reasoner.terminal(*state) ){
                auto reward_0 = reasoner.reward(*r0, state);
                auto reward_1 = reasoner.reward(*r1, state);
                viz.draw(*state);
                std::cout << "Rewards for " << *r0<< " , " << reward_0 << "\n";
                std::cout << "Rewards for " << *r1<< " , " << reward_1 << "\n";
                break;
            }

            viz.draw(*state);

            Moves* moves = reasoner.moves(*state, *r0,false);
            Moves* moves1 = reasoner.moves(*state, *r1,false);
            
            auto amove = ares.makeMove(++seq,prev);
            
            auto j = rand() % moves->size();
            auto i = rand() % moves1->size();

            std::cout << "selecting " << (*moves1)[i]->to_string() << " randomly\n";
            if( amove.first ){
                std::cout << "Ares selected " << amove.first->to_string() << " Seq num: " << amove.second << "\n";
            }
            else{
                std::cout << "selecting randomly for ares" << (*moves)[j]->to_string() << "\n";
            }
            if( prev ) delete prev;
            prev = new Moves{ amove.first ? amove.first : (*moves)[j], (*moves1)[i]};
            
            State* nxt = reasoner.next(*state, *prev);

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
    std::cout << "Total time of program execution : " << dur.count() <<" microseconds\n";
    std::cout << "Average time per step: " << (dur.count()/total_steps) <<" microseconds\n";
    return 0;
}