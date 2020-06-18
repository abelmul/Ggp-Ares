#ifndef GAME_HH
#define GAME_HH
#include <unordered_map>
#include <thread>
#include <iostream>
#include "utils/threading/locks.hh"
namespace ares
{
    typedef const Term Role;
    typedef const Term Move;
    typedef std::shared_ptr<Move> move_sptr;
    typedef std::shared_ptr<Role> role_sptr;
    typedef std::vector<move_sptr> Moves;
    typedef std::vector<role_sptr> Roles;
    
    struct KnowledgeBase
    {
        virtual const std::vector<const Clause*>* operator [](const char* name)const = 0;
        virtual void add(const char* name, Clause*) = 0;

        protected:
            SpinLock slock;
    };
    class Game : public KnowledgeBase
    {
    private:
        /*A mapping from head names --> [clauses with the same head name]*/
        std::unordered_map<const char*, std::vector<const Clause*>*,CharpHasher, StrEq> rules;

    public:
        Game(/* args */){}

        virtual const std::vector<const Clause*>* operator [](const char* name) const {
            const std::vector<const Clause*>* v = nullptr;
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
                rules[strdup(name)] = new std::vector<const Clause*>();
            
            rules[name]->push_back(c);
        }
        
        std::unordered_map<const char *, std::vector<const Clause*>*, CharpHasher,StrEq>::iterator begin(){ return rules.begin();}
        std::unordered_map<const char *, std::vector<const Clause*>*, CharpHasher,StrEq>::iterator end(){ return rules.end();}
        
        std::unordered_map<const char*, std::vector<const Clause*>*,CharpHasher,StrEq> getRules(){
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