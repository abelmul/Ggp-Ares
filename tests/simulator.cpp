#include "ares.hh"
#include "static.hh"

ares::Cfg ares::cfg;
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

    //Create Ares
    Ares& ares( Ares::create(reasoner, Registrar::get(cfg.strategy.c_str()), p));

    //Start game, this is done after the start message has been recieved
    //Create the knowledge base
    Game* kb(new Game());
   
    //Parse the gdl
    p.parse(kb,cfg.gdlFile);
    reasoner.setGame(kb);

    const role_sptr& r0 = reasoner.getRoles()[0];
    const role_sptr& r1 = reasoner.getRoles()[1];

    std::cout << "-----Roles------\n\n";
    std::cout << *r0 << std::endl;
    std::cout << *r1 << std::endl;
    std::cout << "-----Roles------\n\n";
    std::cout << cfg << "\n";
    srand(time(NULL));
    visualizer viz;

    //Get the initial state
    const State& init = reasoner.getInit();
    uint sims = 0;

    auto c = std::chrono::high_resolution_clock();
    auto begin = c.now();
    uint total_steps = 0;
    while (sims < cfg.simulaions)
    {
        const State* state = &init;
        ushort steps=0;
        while (steps < cfg.steps)
        {
            if( reasoner.isTerminal(*state) ){
                auto reward_0 = reasoner.getReward(*r0, state);
                auto reward_1 = reasoner.getReward(*r1, state);
                viz.draw(*state);
                std::cout << "Rewards for " << *r0<< " , " << reward_0 << "\n";
                std::cout << "Rewards for " << *r1<< " , " << reward_1 << "\n";
                break;
            }
            Moves* moves = reasoner.legalMoves(*state, *r0);
            Moves* moves1 = reasoner.legalMoves(*state, *r1);
            int m0=0,m1=0;
            if( cfg.random ){
                m0= rand() % moves->size();
                m1= rand() % moves1->size();
            }
            else takeIn(state,moves,moves1,r0,r1,m0,m1);
            Moves nmoves{(*moves)[m0], (*moves1)[m1]};

            State* nxt = reasoner.getNext(*state, nmoves);
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

void takeIn(const ares::State* state,ares::Moves*  moves, ares::Moves* moves1,const ares::role_sptr& r0,const ares::role_sptr& r1, int& m0,int& m1){
    static auto viz = ares::visualizer();
    viz.draw(*state);
    std::cout << "Legal Moves for : " << *r0 << "\n";
    uint j=1;
    for (uint i=0;i<moves->size();i++){
        std::cout << (boost::format("[%|-2|]%|=25|") % i % (*moves)[i]->to_string());
        if( j%5 == 0) std::cout << "\n";
        j++;
    }
    std::cout <<"\n-------------------------------------------------------\n\n";
    
    j=1;
    std::cout << "\nLegal Moves for : " << *r1 << "\n";
    for (uint i=0;i<moves1->size();i++){
        std::cout << (boost::format("[%|=2|]%|=25|") % i % (*moves1)[i]->to_string());
        if( j%5 == 0) std::cout << "\n";
        j++;
    }
    
    std::cout <<"\n-------------------------------------------------------\n\n";

    if(moves->size() > 1){
        std::cout << "Move "<< *r0 <<": ";
        std::cin >> m0;   
    }
    if(moves1->size() > 1){
        std::cout << "Move "<< *r1 <<": ";
            std::cin >> m1;
    }
}

