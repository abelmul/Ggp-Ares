#include "ares.hh"
#include <unistd.h>
#include <random>
#include <chrono>
ares::Cfg ares::cfg;

int main(int argc, char const *argv[])
{
    using namespace ares;
    Ares ares;
    cfg = Cfg("./ares.cfg.json");
    std::vector<std::pair<arity_t, uint>> arities = {make_pair(1,100),make_pair(2,100),make_pair(3,100),make_pair(4,100),make_pair(5,100),make_pair(6,100),make_pair(7,100),make_pair(8,100)};
    MemoryPool* mempool = new MemoryPool(400,400,arities);
    GdlParser* p = GdlParser::getParser(cfg.parserThreads);
    
    ares.mempool = mempool;
    ares.exprpool = p->getExpressionPool();
    MemoryPool::exprPool = ares.exprpool;

    ClauseBody::mempool = mempool;
    Body::mempool = mempool;

    Game* kb = new Game();
    p->parse(kb,cfg.gdl);

    SuffixRenamer::setPool(p->getExpressionPool());
    Prover* prover = Prover::getProver(kb);
    
    std::cout << "------Knoweledge base-------\n\n";
    for(auto &&i : *kb){
        std::cout << " Key : " << i.first << std::endl;
        for(auto &&j : *i.second)
            std::cout << j->to_string() << std::endl;
    }
    std::cout << "------Knoweledge base-------\n\n";

    Reasoner reasoner(*kb, *p, *prover);
    
    role_sptr r0 = reasoner.roles[0];
    role_sptr r1 = reasoner.roles[1];

    std::cout << "-----Roles------\n\n";
    std::cout << *r0 << std::endl;
    std::cout << *r1 << std::endl;
    std::cout << "-----Roles------\n\n";
    srand(time(NULL));
    const State& init = reasoner.getInit();
    uint sims = 0;
    auto c = std::chrono::high_resolution_clock();
    auto begin = c.now();
    while (sims < cfg.simulaions)
    {
        const State* state = &init;
        while (true)
        {
            // for (auto &&s : *(*state)["true"])
            //     std::cout << *s << "\n";
                
            if( reasoner.isTerminal(*state) ){
                auto reward_0 = reasoner.getReward(*r0, state);
                auto reward_1 = reasoner.getReward(*r1, state);
                std::cout << "Rewards for " << r0 << reward_0 << "\n";
                std::cout << "Rewards for " << r1 << reward_1 << "\n";
                break;
            }
            Moves* moves = reasoner.legalMoves(*state, *r0);
            Moves* moves1 = reasoner.legalMoves(*state, *r1);

            // std::cout << "Legal Moves for : " << *r0 << "\n";
            
            // for (uint i=0;i<moves->size();i++)
            //     std::cout << "\t\t["<<i<<"]"<< *(*moves)[i]  <<"\n";

            // std::cout << "Legal Moves for : " << r1 << "\n";
            // for (uint i=0;i<moves1->size();i++)
            //     std::cout << "\t\t["<<i<<"]"<< *(*moves1)[i]  <<"\n";

            int m0= rand() % moves->size();
            int m1= rand() % moves1->size();
            // if(moves->size() > 1){
            //     std::cout << "Move "<< *r0 <<": ";
            //     std::cin >> m0;   
            // }
            // if(moves1->size() > 1){
            //     std::cout << "Move "<< *r1 <<": ";
            //      std::cin >> m1;
            // }

            Moves nmoves{(*moves)[m0], (*moves1)[m1]};

            State* nxt = reasoner.getNext(*state, nmoves);
            MemoryPool::remove(moves,true);
            MemoryPool::remove(moves1,true);
            
            if( state != &init ) delete state;
            state = nxt;
            // return 0;
            sims++;
        }
    }
    auto end = c.now();
    auto dur = std::chrono::duration_cast<std::chrono::milliseconds>(end-begin);
    std::cout <<cfg.proverThreads <<" threads." <<"\n";
    std::cout << cfg.simulaions << " simulations in : " << dur.count()<<"milliseconds\n";
    std::cout << "a simulation in : " << dur.count()/cfg.simulaions<<"milliseconds\n";
    return 0;
}

namespace ares
{
    ExpressionPool* Ares::exprpool = nullptr;
    MemoryPool* Ares::mempool = nullptr;
    
    //Initialize static members of term
    CharpHasher Term::nameHasher;
    cnst_term_sptr     Term::null_term_sptr(nullptr);
    cnst_lit_sptr     Term::null_literal_sptr(nullptr);
    
    template<class T>
    MemoryPool* _Body<T>::mempool =nullptr;

} // namespace ares
