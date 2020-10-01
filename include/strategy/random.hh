#ifndef RANDOM_STR_HH
#define RANDOM_STR_HH
#include "strategy.hh"
#include "utils/utils/cfg.hh"

namespace ares{
    #define INIT ((State*)0x1)
    class Random : public Strategy, public RegistrarBase<Random>
    {
    private:
        /* data */
    public:
        Random(){}
        virtual move_sptr_seq operator()(const Match& match,uint seq){
            const State* state;
            if( current == nullptr ){
                state = match.game->init();
                current = INIT;
            }
            else if(current == INIT)
                current = state = reasoner->next(*match.game->init(), *match.takenAction);
            
            else{
                state = reasoner->next(*current, *match.takenAction);
                delete current;
                current = state;
            }
            log("[RandomStrategy]") << "Selecting a move\n";
            return move_sptr_seq(reasoner->randMove(*state,*match.role),seq);
        }
        virtual void start(const Match&){}
        virtual void reset(){
            if(current and current != INIT) delete current; 
            current= nullptr;
        }
        virtual std::string name() { return "Random";};
        
        //Singleton
        static Random* create(){ static Random random; return &random;}

        ~Random() {
            log("[~Random]");
            reset();
        }
    };
}
#endif