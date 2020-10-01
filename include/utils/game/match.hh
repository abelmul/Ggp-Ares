#ifndef MATCH_HH
#define MATCH_HH

#include "utils/gdl/clause.hh"
#include "utils/game/game.hh"

namespace ares
{
    class State;
    
    struct Match
    {
        Match():takenAction(nullptr) {}
        ~Match() {}
        void reset(){
            game = nullptr;
            matchId = "";
            role = nullptr;
            strtClck=0;
            plyClck=0;
        }
        Game* game;
        std::string matchId;
        uint strtClck;
        uint plyClck;
        Action* takenAction;
        Role* role;
    };
    /**
     * Represents the match's state.
     */
    class State : public KnowledgeBase
    {
    private:
        std::unordered_map<ushort, UniqueClauseVec*> state;
        
    public:
        State(){

        }
        virtual const UniqueClauseVec* operator [](ushort name) const {
            const UniqueClauseVec* v = nullptr;
            try
            {
                v = state.at(name);
            }
            catch(const std::exception& e){}
            
            return v;
        }
        virtual bool add(ushort name, Clause* c){
            std::lock_guard<SpinLock> lk(slock);
            if( state.find(name) == state.end() )
                state[name] = new UniqueClauseVec();
            
            return state[name]->push_back(c);
        }
        std::unordered_map<ushort, UniqueClauseVec*>::const_iterator begin()const
        { return state.cbegin();}
        std::unordered_map<ushort, UniqueClauseVec*>::const_iterator end()const
        { return state.cend();}
        
        State& operator +=(const State& s){
            state.insert(s.state.begin(), s.state.end());
            return *this;
        }
        void reset() { state.clear();}
        State* clone()const{
            auto sClone = new State();
            for (auto &&[name,vec] : state)
            {
                auto& stateC =sClone->state[name];
                stateC = new UniqueClauseVec(vec->size());
                for (size_t i = 0; i < vec->size(); i++)
                    stateC->push_back((*vec)[i]->clone());
            }
            
            return sClone; 
        }
        std::string toString()const{
            std::string s;
            for (auto &&[name,vec] : state)
            {
                s.append( "----" + std::to_string(name) + "-----\n");
                for (auto &&i : *vec)
                    s.append( i->to_string() + "\n") ;
            }   
            return s;
        }
        std::string toStringHtml()const{
            std::string s;
            for (auto &&[name,vec] : state)
            {
                for (auto &&i : *vec)
                    s.append( i->to_string() + "<br/>") ;
            }   
            return s;
        }
        bool operator ==(const State& other){
            if( state.size() != other.state.size() ) return false;
            for (auto &&[name,vec] : state)
            {
                auto it =  other.state.find(name);
                if( it == other.state.end() or ( (*vec) != (*it->second) ) ) return false;
            }
            return true;
        }
        virtual ~State(){
            for (auto &&[name, vector] : state)
            {
                for (auto &&c : *vector)
                    delete c;
                
                delete vector;
            }
            
        }
    };
    
} // namespace ares

#endif