#ifndef GAME_HH
#define GAME_HH
#include <unordered_map>
#include <thread>
#include <iostream>
#include "utils/threading/locks.hh"
#include "utils/utils/hashing.hh"

namespace ares
{
    typedef const Term Move;
    typedef const Term Role;
    typedef cnst_term_sptr move_sptr;
    typedef cnst_term_sptr role_sptr;
    typedef std::vector<move_sptr> Moves;
    typedef std::vector<role_sptr> Roles;
    
    typedef UniqueVector<const Clause*,ClauseHasher,ClauseHasher> UniqueClauseVec;

    struct KnowledgeBase
    {
        virtual const UniqueClauseVec* operator [](const char* name)const = 0;
        virtual void add(const char* name, Clause*) = 0;

        protected:
            SpinLock slock;
    };
    class Game : public KnowledgeBase
    {
    private:
        /*A mapping from head names --> [clauses with the same head name]*/
        std::unordered_map<const char*, UniqueClauseVec*,CharpHasher, StrEq> rules;

    public:
        Game(/* args */){}

        virtual const UniqueClauseVec* operator [](const char* name) const {
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
        
        virtual void add(const char* name, Clause* c){
            std::lock_guard<SpinLock> lk(slock);
            if( rules.find(name) == rules.end() )
                rules[strdup(name)] = new UniqueClauseVec();
            
            rules[name]->push_back(c);
        }
        
        std::unordered_map<const char *, UniqueClauseVec*, CharpHasher,StrEq>::iterator begin(){ return rules.begin();}
        std::unordered_map<const char *, UniqueClauseVec*, CharpHasher,StrEq>::iterator end(){ return rules.end();}
        
        std::unordered_map<const char*, UniqueClauseVec*,CharpHasher,StrEq> getRules(){
            return rules;
        }
        
        ~Game(){
            for (auto& it : rules)
            {
                delete it.first;
                for (const Clause *c : *it.second)
                        delete c;
                delete it.second;
            }   
        }
    };
} // namespace ares

#endif