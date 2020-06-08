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
        virtual void add(const char* name, Clause*) = 0;

        protected:
            SpinLock slock;
    };

    /**
     * Represents the game's state.
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