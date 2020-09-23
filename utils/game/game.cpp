#include "utils/game/game.hh"
#include "ares.hh"

namespace ares{
    const Roles& Game::getRoles(){
        if( roles.size() == 0 )
            for (auto &&role : *rules[Namer::ROLE])
                roles.push_back(role->getHead()->getBody()[0]);
        
        return roles;
    }
    const State* Game::init(){
        //Just change the init(...) relations to true(...) relations
        if( not init_)
        {
            init_ = new State();
            for (auto &&s : *rules[Namer::INIT])
            {
                auto& body = s->getHead()->getBody();
                PoolKey key{Namer::TRUE, new Body(body.begin(),body.end()),true};
                init_->add(Namer::TRUE, new Clause(Ares::memCache->getLiteral(key),new ClauseBody(0)));
            }
        }
        return init_;
    }
    inline Game::~Game(){
        for (auto& it : rules)
        {
            for (const Clause *c : *it.second)
                    delete c;
            delete it.second;
        }
        if( init_ )
            delete init_;   
        log("[~Game]");
    }
};