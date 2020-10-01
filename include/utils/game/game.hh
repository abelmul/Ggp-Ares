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
    typedef std::vector<Move*> Moves;
    typedef std::vector<Role*> Roles;
    typedef Moves Action;
    typedef std::unique_ptr<Action> uAction;
    typedef std::unique_ptr<const Action> ucAction;
    typedef std::unique_ptr<Moves> uMoves;

    
    typedef UniqueVector<const Clause*,ClauseHasher,ClauseHasher> UniqueClauseVec;

    struct KnowledgeBase
    {
        KnowledgeBase(){}
        KnowledgeBase(const KnowledgeBase&)=delete;
        KnowledgeBase& operator=(const KnowledgeBase&)=delete;
        KnowledgeBase(const KnowledgeBase&&)=delete;
        KnowledgeBase& operator=(const KnowledgeBase&&)=delete;
        
        virtual const UniqueClauseVec* operator [](ushort name)const = 0;
        virtual bool add(ushort name, Clause*) = 0;
        virtual ~KnowledgeBase(){}
        protected:
            SpinLock slock;
    };
    class Game : public KnowledgeBase
    {
    private:
        /*A mapping from head names --> [clauses with the same head name]*/
        std::unordered_map<ushort, UniqueClauseVec*> rules;
        Roles roles_;
        State* init_;

    public:
        Game(/* args */):init_(nullptr){}

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
        
        virtual bool add(ushort name, Clause* c){
            std::lock_guard<SpinLock> lk(slock);
            if( rules.find(name) == rules.end() )
                rules[name] = new UniqueClauseVec();
            
            return rules[name]->push_back(c);
        }
        
        std::unordered_map<ushort, UniqueClauseVec*>::iterator begin(){ return rules.begin();}
        std::unordered_map<ushort, UniqueClauseVec*>::iterator end(){ return rules.end();}
        
        std::unordered_map<ushort, UniqueClauseVec*> getRules(){
            return rules;
        }
        /**
         * The roles are static wihin a game no need to compute them.
         */
        const Roles& roles(){return roles_;}
        void addRole(const Role* r){roles_.push_back(r);}
        /**
         * set the initial state
         */
        void init(State* s){ init_ = s;}
        /**
         * get the initial state
         */
        const State* init(){ return init_;}

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