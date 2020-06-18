#include "ares.hh"
#include <unistd.h>
#include <random>
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
    Prover* prover = Prover::getProver(kb, cfg.proverThreads, cfg.negThreads);
    
    std::cout << "------Knoweledge base-------\n\n";
    for(auto &&i : *kb){
        std::cout << " Key : " << i.first << std::endl;
        for(auto &&j : *i.second)
            std::cout << j->to_string() << std::endl;
    }
    std::cout << "------Knoweledge base-------\n\n";

    Reasoner reasoner(*kb, *p, *prover);
    
    std::cout << "-----Roles------\n\n";
    std::cout << *reasoner.roles[0] << std::endl;
    std::cout << *reasoner.roles[1] << std::endl;
    std::cout << "-----Roles------\n\n";

    

    
    std::vector<std::vector<std::pair<std::string, std::string>>> record;
    srand(time(NULL));
    const State& init = reasoner.getInit();
    while (true)
    {
        int l;
        std::cout <<"Continue: ";
        // std::cin >> l;
        // std::cout <<"\n";
        const State* state = &init;
        record.push_back(std::vector<std::pair<std::string, std::string>>());
        while (true)
        {

            std::cout << "------state -------\n\n";
            for (auto &&i : *state)
            {
                for (auto &&j : *i.second)
                {
                    std::cout << *j << std::endl;
                }

            }
            std::cout << "------state -------\n\n";
            std::cout << "------isTerminal -------\n\n";
            if( reasoner.isTerminal(*state) ){
                auto reward_0 = reasoner.getReward(*reasoner.roles[0], state);
                auto reward_1 = reasoner.getReward(*reasoner.roles[1], state);
                std::cout << "Rewards for " << *reasoner.roles[0] << " : " << reward_0 << std::endl;
                std::cout << "Rewards for " << *reasoner.roles[1] << " : " << reward_1 << std::endl;
                break;
            }
            std::cout << "------isTerminal -------\n\n";
            printf("Reasoner is at : %p, ", &reasoner);
            std::cout << "--- Legal Moves for " << *reasoner.roles[0] << "----\n";
            Moves* moves = reasoner.legalMoves(*state, *reasoner.roles[0]);
            for (uint i=0; i < moves->size() ;i++){
                auto m = (* moves)[i];
                std::cout << "["<<i<<"]"<< m->to_string() << std::endl;
            }
            std::cout << "--- Legal Moves for " << *reasoner.roles[0] << "----\n";

            fflush(NULL);
            
            std::cout << "--- Legal Moves for " << *reasoner.roles[1] << "----\n";
            Moves* moves1 = reasoner.legalMoves(*state, *reasoner.roles[1]);

            for (uint i=0; i < moves1->size() ;i++){
                auto m = (* moves1)[i];
                std::cout << "["<<i<<"]"<< m->to_string() << std::endl;
            }
            std::cout << "--- Legal Moves for " << *reasoner.roles[1] << "----\n";


            int m0 = rand() % moves->size();
            int m1 = rand() % moves1->size();
            record.back().push_back(make_pair((*moves)[m0]->to_string(), (*moves1)[m1]->to_string()));
            std::cout << "Move " << *reasoner.roles[0] << ": " << (*moves)[m0]->to_string()   << "\n";
            std::cout << "Move " << *reasoner.roles[1] << ": " <<  (*moves1)[m1]->to_string() << "\n";

            Moves nmoves{(*moves)[m0], (*moves1)[m1]};

            State* nxt = reasoner.getNext(*state, nmoves);
            printf("\n\nAfter next ...\n");
            // delete moves;
            // delete moves1;
            // delete state;
            state = nxt;
            // return 0;
        }
    }
    
    
    
    return 0;
}

namespace ares
{
    ExpressionPool* Ares::exprpool = nullptr;
    MemoryPool* Ares::mempool = nullptr;
    
    //Initialize static members of term
    CharpHasher Term::nameHasher;
    std::shared_ptr<const Term>     Term::null_term_sptr(nullptr);
    template<class T>
    MemoryPool* _Body<T>::mempool =nullptr;

} // namespace ares
