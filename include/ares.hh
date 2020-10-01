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
        Ares(Strategy& strategy_,Reasoner& reasoner_)
        :parser(GdlParser::create(memCache))
        ,reasoner(reasoner_)
        ,strategy(strategy_)
        {
            Registrar::init(&reasoner);
        }

    public:
        Reasoner* operator->(){ return &reasoner;}

        inline void startMatch(Match& m,const std::string& role){
            match = m;
            match.role =  memCache->getConst(Namer::id(role));
            reasoner.reset(m.game);
            strategy.start(match);
        }
        
        inline std::pair<Move*,uint> makeMove(uint seq,Moves* moves=nullptr){
            if( match.takenAction ) delete match.takenAction;
            match.takenAction = moves;
            auto move = strategy(match,seq);
            return move;
        }

        inline const std::string& currentMatch() { return match.matchId;}
        
        inline bool abortMatch(const std::string& id){
            if( match.matchId != id ) return false;
            if( match.takenAction  ) {delete match.takenAction; match.takenAction=nullptr;}
            strategy.reset();
            match.reset();
            reasoner.reset(nullptr);
            return true;
        }

        inline void stopMatch(Moves* moves){
            //Compute the current state
            if( match.takenAction  ) {delete match.takenAction; match.takenAction=nullptr;}
            auto* current = reasoner.next(strategy.matchState(),*moves);

            //Just display the rewards, if its a terminal state
            if( reasoner.terminal(*current) ){
                log("[Ares] Rewards: ");
                std::string sep("");
                for (auto &&role : reasoner.roles() )
                {
                    std::cout << sep ;
                    log(Namer::name(role->get_name())) << ", ";
                    log(to_string(reasoner.reward(*role, current)));
                    sep = " | ";
                }
                std::cout << "\n";
            }
            delete current;
            // //Delete the knowledgebase
            match.reset();
            strategy.reset();
            reasoner.reset(nullptr);
        }
        static Ares& create(Strategy& strategy,Reasoner& reasoner_){
            static Ares ares(strategy,reasoner_);
            return ares;
        }

        static void setMem(MemoryPool* mem){
            mempool = mem;
            memCache = mem->getCache();
        }
        ~Ares(){}

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