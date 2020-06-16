#ifndef GAME_HH
#define GAME_HH
#include <unordered_map>
#include <thread>

namespace Ares
{
    typedef Term Move;     
    struct SpinLock
    {
        void lock(){
            while (_lock.test_and_set(std::memory_order_acquire));
        }
        void unlock(){
            _lock.clear(std::memory_order_release); 
        }

        private:
            std::atomic_flag _lock = ATOMIC_FLAG_INIT;
    };
    
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
        std::unordered_map<const char*, std::vector<const Clause*>*,CharpHasher> rules;

    public:
        Game(/* args */){}

        virtual const std::vector<const Clause*>* operator [](const char* name) const {
            return rules.at(name);
        }
        
        virtual void add(const char* name, Clause* c){
            slock.lock();
            if( rules.find(name) == rules.end() )
                rules[name] = new std::vector<const Clause*>();
            
            rules[name]->push_back(c);
            slock.unlock();
        }
        std::unordered_map<const char*, std::vector<const Clause*>*,CharpHasher> getRules(){
            return rules;
        }
        
        ~Game(){
            for (auto& it : rules)
            {
                for (const Clause *c : *it.second)
                        delete c;
                delete it.second;
            }   
        }
    };
} // namespace Ares

#endif