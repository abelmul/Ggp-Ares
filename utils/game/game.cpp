#include "utils/game/game.hh"
#include "ares.hh"

namespace ares{
    Game::~Game(){
        for (auto& it : rules)
        {
            for (const Clause *c : *it.second)
                    delete c;
            delete it.second;
        }
        if( init_ )
            delete init_;   
    }
};