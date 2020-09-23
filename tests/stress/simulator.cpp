#include "ares.hh"
#include "static.hh"
#include "strategy/random.hh"
#include "strategy/montecarlo.hh"

ares::Cfg ares::cfg;
void takeIn(const ares::State* state,std::vector<ares::uMoves>*  moves,int& m);
void takeIn(const ares::State* state,ares::Moves*  moves, ares::Moves* moves1,const ares::role_sptr& r0,const ares::role_sptr& r1, int& m0,int& m1);
using namespace ares;

/**
 * This test is inteded to 
 */
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
    Reasoner& reasoner(Reasoner::create(GdlParser::create(mempool.getCache()), Prover::create(), *mempool.getCache()));
    Ares& ares( Ares::create(Registrar::get(cfg.strategy.c_str()),reasoner));

    //Start game, this is done after the start message has been recieved
    //Create the knowledge base
    Game* kb(new Game());
   
    //Parse the gdl
    ares.parser.parse(kb,cfg.gdlFile);
    ares->reset(kb);

    const role_sptr& r0 = ares->roles()[0];
    const role_sptr& r1 = ares->roles()[1];

    std::cout << "-----Roles------\n\n";
    std::cout << *r0 << std::endl;
    std::cout << *r1 << std::endl;
    std::cout << "-----Roles------\n\n";
    srand(time(NULL));
    visualizer viz;

    //Get the initial state
    const State& init = ares->init();
    uint sims = 0;

    auto c = std::chrono::high_resolution_clock();
    auto begin = c.now();
    uint total_steps = 0;
    while (sims < cfg.simulations)
    {
        const State* state = &init;
        ushort steps=0;
        while (steps < cfg.steps)
        {
            if( ares->terminal(*state) ){
                auto reward_0 = ares->reward(*r0, state);
                auto reward_1 = ares->reward(*r1, state);
                viz.draw(*state);
                std::cout << "Rewards for " << *r0<< " , " << reward_0 << "\n";
                std::cout << "Rewards for " << *r1<< " , " << reward_1 << "\n";
                break;
            }
            
            Action* action;
            if( !cfg.random ){
                int m0=0,m1=0;
                auto moves= ares->moves(*state, *r0,false);
                auto moves1= ares->moves(*state, *r1,false);
                takeIn(state,moves,moves1,r0,r1,m0,m1);
                action = new Action{(*moves)[m0],(*moves1)[m1]};
                delete moves;
                delete moves1;
            }
            else 
                action = ares->randAction(*state);
            
            State* nxt = ares->next(*state, *action);
            delete action;
            
            if( state != &init ) delete state;
            state = nxt;
            // return 0;
            steps++;
        }
        std::cout << "Steps : " << steps << "\n";
        sims++;
        if( state != &init ) delete state;
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

void takeIn(const ares::State* state,std::vector<ares::uMoves>*  moves,int& m){
    static auto viz = ares::visualizer();
    viz.draw(*state);
    std::cout << "Legal Actions" <<"\n";
    uint j=1;
    for (auto &&action : *moves){
        std::string s,sep;
        for (auto &&a : *action)
        {
            s+= sep + a->to_string();
            sep = ",";
        }
        if( j%5 == 0) std::cout << "\n";
        std::cout << (boost::format("[%|-2|]%|=32|") % (j-1) % s);
        j++;
    }
    std::cout <<"\n-------------------------------------------------------\n\n";

    if(moves->size() > 1){
        std::cout << "Selected Action "<< ": ";
        std::cin >> m;   
    }
}
