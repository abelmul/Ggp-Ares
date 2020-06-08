#ifndef GAME_HH
#define GAME__HH
#include "utils/game/state.hh"
#include <unordered_map>

namespace Ares
{
    class Game : public KnowledgeBase
    {
    private:
        /*A mapping from head names --> [clauses with the same head name]*/
        std::unordered_map<const char*, std::vector<Clause*>*,CharpHasher> rules;
        State* state;

    public:
        Game(/* args */){}
        void setState(State* nState){
        }

        virtual std::vector<Clause*>* operator [](char* name){
            return rules[name];
        }
        
        virtual void add(const char* name, Clause* c){
            slock.lock();
            if( rules.find(name) == rules.end() )
                rules[name] = new std::vector<Clause*>();
            
            rules[name]->push_back(c);
            slock.unlock();
        }
        ~Game(){
            for (auto& it : rules)
            {
                for (Clause *c : *it.second)
                        delete c;
                delete it.second;
            }   
        }
    };
} // namespace Ares

#endif