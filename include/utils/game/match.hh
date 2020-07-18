#ifndef MATCH_HH
#define MATCH_HH

#include "utils/gdl/clause.hh"
#include "utils/game/game.hh"

namespace ares
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
        std::unordered_map<const char *, UniqueClauseVec*, CharpHasher,StrEq> state;
        
    public:
        State(){

        }
        virtual const UniqueClauseVec* operator [](const char* name) const {
            const UniqueClauseVec* v = nullptr;
            try
            {
                v = state.at(name);
            }
            catch(const std::exception& e){}
            
            return v;
        }
        virtual void add(const char* name, Clause* c){
            std::lock_guard<SpinLock> lk(slock);
            if( state.find(name) == state.end() )
                state[strdup(name)] = new UniqueClauseVec();
            
            state[name]->push_back(c);
        }
        std::unordered_map<const char *, UniqueClauseVec*, CharpHasher>::const_iterator begin()const
        { return state.cbegin();}
        std::unordered_map<const char *, UniqueClauseVec*, CharpHasher>::const_iterator end()const
        { return state.cend();}
        
        State& operator +=(const State& s){
            state.insert(s.state.begin(), s.state.end());
            return *this;
        }
        virtual ~State(){
            for (auto &&i : state)
            {
                delete i.first;
                for (auto &&c : *i.second)
                    delete c;
                
                delete i.second;
            }
            
        }
    };
    
} // namespace ares

#endif