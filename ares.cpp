#include "ares.hh"

ares::Cfg ares::cfg;
void takeIn(const ares::State* state,ares::Moves*  moves, ares::Moves* moves1,ares::role_sptr& r0,ares::role_sptr& r1, int& m0,int& m1);
int main(int argc, char const *argv[])
{
    using namespace ares;
    //Read in the configuration file
    cfg = Cfg("./ares.cfg.json");

    //Create the memory pool
    std::vector<std::pair<arity_t, uint>> arities = {make_pair(1,32768),make_pair(2,32768),make_pair(3,32768),make_pair(4,32768),make_pair(5,32768),make_pair(6,32768),make_pair(7,32768),make_pair(8,32768),make_pair(9,4096),make_pair(10,32768),make_pair(11,4096),make_pair(12,4096)};
    Ares ares(new MemoryPool(131072,262144,arities));

    //Create the parser
    GdlParser* p = GdlParser::getParser(cfg.parserThreads,ares.memCache);

    //Setup some static elements
    Body::mempool = ClauseBody::mempool = ares.mempool;

    //Create the knowledge base
    Game* kb(new Game());
    auto c = std::chrono::high_resolution_clock();
    auto begin = c.now();

    //Parse the gdl
    p->parse(kb,cfg.gdl);
    SuffixRenamer::setPool(ares.memCache);

    //Get the singleton prover
    Prover* prover(Prover::getProver(kb));
    ClauseCB::prover = prover;
    // std::cout << "------Knoweledge base-------\n\n";
    // for(auto &&i : *kb){
    //     std::cout << " Key : " << i.first << std::endl;
    //     for(auto &&j : *i.second)
    //         std::cout << j->to_string() << std::endl;
    // }
    // std::cout << "------Knoweledge base-------\n\n";

    //Create a reasoner over the game
    Reasoner* _reasoner(new Reasoner(*kb, *p, *prover,*ares.memCache));
    Reasoner& reasoner = *_reasoner;
    
    role_sptr& r0 = reasoner.roles[0];
    role_sptr& r1 = reasoner.roles[1];

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
    }
    auto end = c.now();
    auto dur = std::chrono::duration_cast<std::chrono::microseconds>(end-begin);
    std::cout << "Total time of program execution : " << dur.count() <<" microseconds\n";

    delete kb;
    delete p;
    delete _reasoner;
    delete prover;
    
    return 0;
}

void takeIn(const ares::State* state,ares::Moves*  moves, ares::Moves* moves1,ares::role_sptr& r0,ares::role_sptr& r1, int& m0,int& m1){
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

// voi
namespace ares
{
    Prover* ClauseCB::prover;
    MemCache* Ares::memCache = nullptr;
    MemoryPool* Ares::mempool = nullptr;
    
    //Initialize static members of term
    cnst_term_sptr     Term::null_term_sptr(nullptr);
    cnst_lit_sptr     Term::null_literal_sptr(nullptr);
    
    //Namer static
    std::unordered_map<ushort, std::string> Namer::vIdName;
    std::unordered_map<std::string, ushort> Namer::vNameId;
    
    std::unordered_map<ushort, std::string> Namer::idName;
    std::unordered_map<std::string, ushort> Namer::nameId;

    const ushort Namer::ROLE = Namer::registerName(std::string("role"));
    const ushort Namer::INIT = Namer::registerName(std::string("init"));
    const ushort Namer::LEGAL = Namer::registerName(std::string("legal"));
    const ushort Namer::NEXT = Namer::registerName(std::string("next"));
    const ushort Namer::TRUE = Namer::registerName(std::string("true"));
    const ushort Namer::DOES = Namer::registerName(std::string("does"));
    const ushort Namer::DISTINCT = Namer::registerName(std::string("distinct"));
    const ushort Namer::GOAL = Namer::registerName(std::string("goal"));
    const ushort Namer::TERMINAL = Namer::registerName(std::string("terminal"));
    const ushort Namer::INPUT = Namer::registerName(std::string("input"));
    const ushort Namer::BASE = Namer::registerName(std::string("base"));
    const ushort Namer::X = Namer::registerVname(std::string("?x"));
    const ushort Namer::R = Namer::registerVname(std::string("?r"));
    
    template<class T>
    MemoryPool* _Body<T>::mempool =nullptr;

    std::ostream& operator<<(std::ostream& os, const Cfg& cfg){
        os << cfg.str();
        return os;
    }
} // namespace ares
