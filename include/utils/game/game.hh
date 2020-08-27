#ifndef GAME_HH
#define GAME_HH
#include <unordered_map>
#include <thread>
#include <iostream>
#include "utils/threading/locks.hh"
#include "utils/utils/hashing.hh"
#include "utils/memory/namer.hh"
#include "utils/gdl/clause.hh"

namespace ares
{
    class State;
    typedef const Term Move;
    typedef const Term Role;
    typedef cnst_term_sptr move_sptr;
    typedef cnst_term_sptr role_sptr;
    typedef std::vector<move_sptr> Moves;
    typedef std::vector<role_sptr> Roles;
    
    typedef UniqueVector<const Clause*,ClauseHasher,ClauseHasher> UniqueClauseVec;

    struct KnowledgeBase
    {
        virtual const UniqueClauseVec* operator [](ushort name)const = 0;
        virtual void add(ushort name, Clause*) = 0;
        virtual ~KnowledgeBase(){}
        protected:
            SpinLock slock;
    };
    class Game : public KnowledgeBase
    {
    private:
        /*A mapping from head names --> [clauses with the same head name]*/
        std::unordered_map<ushort, UniqueClauseVec*> rules;
        Roles roles;
        State* init;

    public:
        Game(/* args */):init(nullptr){}

        virtual const UniqueClauseVec* operator [](ushort name) const {
            const UniqueClauseVec* v = nullptr;
            try
            {
                v = rules.at(name);
            }
            catch(const std::exception& e)
            {
                std::cerr << e.what() << '\n';
            }
            
            return v;
        }
        
        virtual void add(ushort name, Clause* c){
            std::lock_guard<SpinLock> lk(slock);
            if( rules.find(name) == rules.end() )
                rules[name] = new UniqueClauseVec();
            
            rules[name]->push_back(c);
        }
        
        std::unordered_map<ushort, UniqueClauseVec*>::iterator begin(){ return rules.begin();}
        std::unordered_map<ushort, UniqueClauseVec*>::iterator end(){ return rules.end();}
        
        std::unordered_map<ushort, UniqueClauseVec*> getRules(){
            return rules;
        }
        /**
         * The roles are static wihin a game no need to compute them.
         */
        const Roles& getRoles();
        /**
         * THe initial state is static within the game no need to compute it.
         */
        const State* getInit();
        std::string toString(){
            std::string s;
            for (auto &&[name,vec] : rules)
            {
                for (auto &&i : *vec)
                    s.append( i->to_string() + " ") ;
            }   
            return s;
        }
        virtual ~Game();
    };
} // namespace ares

#endif