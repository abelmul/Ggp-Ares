#ifndef ARES_HH
#define ARES_HH

#include "utils/game/visualizer.hh"
#include "reasoner/suffixRenamer.hh"
#include "reasoner/reasoner.hh"
#include "utils/gdl/gdlParser/gdlParser.hh"
#include "utils/game/game.hh"
#include "utils/game/match.hh"
#include "utils/utils/cfg.hh"
#include "utils/memory/memoryPool.hh"
#include "utils/memory/namer.hh"
#include <thread>
#include <chrono> 
#include <fstream>  
#include <iostream>
#include <unistd.h>
#include <random>
namespace ares
{
    class Ares
    {
    private:
        /* data */
        Ares(Reasoner* rsnr , GdlParser* p)
        : parser(p),reasoner(rsnr)
        {
        }

    public:
        Reasoner* operator->(){ return reasoner;}

        inline void startMatch(Match& m,const std::string& role){
            match = m;
            match.role =  memCache->getConst(Namer::id(role));
            reasoner->setGame(m.game);
        }
        inline cnst_term_sptr makeMove(){
            return makeMove(Moves());
        }
        inline cnst_term_sptr makeMove(const Moves& moves){
            auto* nextState = moves.size()==0 ? match.state : reasoner->getNext(*match.state, moves);


            if( match.state != match.game->getInit() )
                delete match.state;

            match.state = nextState;
            //make a random move for now
            auto* legals = reasoner->legalMoves(*nextState, *match.role);
            auto i = rand() %  legals->size();
            auto selected = (*legals)[i];
            delete legals;
            return selected;
        }

        inline const std::string& currentMatch() { return match.matchId;}
        inline const State& getMatchState() { return *match.state;}
        inline void stopMatch(){
            match.reset();
            reasoner->setGame(nullptr);
        }
        static Ares* getAres(Reasoner* rsnr , GdlParser* p){
            {
                std::lock_guard<SpinLock> lk(sl);
                if( not ares )
                    ares = new Ares(rsnr,p);
            }
            return ares;
        }

        static void setMem(MemoryPool* mem){
            mempool = mem;
            memCache = mem->getCache();
        }
        ~Ares(){
            delete reasoner;
            delete parser;
            delete mempool;
        }

        static MemoryPool* mempool;
        static MemCache* memCache;
        GdlParser* parser;

    private:
        Reasoner* reasoner;
        Match match;
        static SpinLock sl;
        static Ares* ares;
    };
} // namespace ares

#endif  