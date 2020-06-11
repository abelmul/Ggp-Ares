#ifndef MATCH_HH
#define MATCH_HH

#include "utils/gdl/clause.hh"
#include "utils/game/game.hh"

namespace Ares
{
    class State;
    
    class Match
    {
    private:
        Game* game;
        State* state;
        uint strtClck;
        uint plyClck;
        std::string role;

    public:
        Match(/* args */) {}
        ~Match() {}
    };
    /**
     * Represents the match's state.
     */
    class State : public KnowledgeBase
    {
    private:
        std::unordered_map<const char *, std::vector<Clause*>*, CharpHasher> state;
        
    public:
        State(){

        }
        virtual std::vector<Clause*>* operator [](char* name){
            return state[name];
        }
        virtual void add(const char* name, Clause* c){
            slock.lock();
            if( state.find(name) == state.end() )
                state[name] = new std::vector<Clause*>();
            
            state[name]->push_back(c);
            slock.unlock();
        }
        ~State();
    };
    
} // namespace Ares

#endif