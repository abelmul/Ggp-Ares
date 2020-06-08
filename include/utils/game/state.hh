#ifndef STATE_HH
#define STATE_HH

#include "utils/gdl/clause.hh"
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
        virtual std::vector<Clause*>* operator [](char* name) = 0;
        virtual void add(char* name, Clause*) = 0;

        protected:
            SpinLock slock;
    };

    /**
     * Represents the game's state.
     */
    class State : public KnowledgeBase
    {
    private:
        std::unordered_map<char *, std::vector<Clause*>*, CharpHasher> base;
        
    public:
        State(){

        }
        virtual std::vector<Clause*>* operator [](char* name){
            return base[name];
        }
        virtual void add(char* name, Clause* c){
            base[name]->push_back(c);
        }
        ~State();
    };
    
} // namespace Ares

#endif