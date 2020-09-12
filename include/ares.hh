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
#include "strategy/strategy.hh"
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
        Ares(Reasoner& rsnr ,Strategy& st, GdlParser& p)
        : parser(p),reasoner(rsnr), strategy(st)
        {
        }

    public:
        Reasoner* operator->(){ return &reasoner;}

        inline void startMatch(Match& m,const std::string& role){
            match = m;
            match.role =  memCache->getConst(Namer::id(role));
            reasoner.setGame(m.game);
        }
        inline cnst_term_sptr makeMove(){
            return makeMove(Moves());
        }
        inline cnst_term_sptr makeMove(const Moves& moves){
            auto* currentState = moves.size()==0 ? match.state : reasoner.getNext(*match.state, moves);

            if( match.state != match.game->getInit() )
                delete match.state;

            match.state = currentState;
            return strategy(match,reasoner);
        }

        inline const std::string& currentMatch() { return match.matchId;}
        inline const State& getMatchState() { return *match.state;}
        
        inline bool abortMatch(const std::string& id){
            if( match.matchId != id ) return false;
            match.reset();
            reasoner.setGame(nullptr);
            return true;
        }

        inline void stopMatch(const Moves& moves){
            //Compute the current state
            auto* current = reasoner.getNext(*match.state,moves);

            if( match.state != match.game->getInit() )
                delete match.state;

            match.state = current;

            //Just display the rewards, if its a terminal state
            if( reasoner.isTerminal(*match.state) ){
                log("[Ares] Rewards: ");
                std::string sep("");
                for (auto &&role : match.game->getRoles() )
                {
                    std::cout << sep ;
                    log(Namer::name(role->get_name())) << ", ";
                    log(to_string(reasoner.getReward(*role, match.state)));
                    sep = " | ";
                }
                std::cout << "\n";
            }

            //Delete the knowledgebase
            match.reset();
            reasoner.setGame(nullptr);
        }
        static Ares& create(Reasoner& rsnr ,Strategy& st, GdlParser& p){
            static Ares ares(rsnr,st,p);
            return ares;
        }

        static void setMem(MemoryPool* mem){
            mempool = mem;
            memCache = mem->getCache();
        }
        ~Ares(){
        }

        static MemoryPool* mempool;
        static MemCache* memCache;
        GdlParser& parser;

    private:
        Reasoner& reasoner;
        Strategy& strategy;
        Match match;
    };
} // namespace ares

#endif  