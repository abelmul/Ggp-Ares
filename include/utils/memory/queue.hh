#ifndef QUEUE_HH
#define QUEUE_HH
#include "utils/gdl/gdl.hh"
#include <unordered_set>
namespace ares
{
    
    struct DeletionQueue{
        inline void enqueue(const structured_term* st){ 
            std::lock_guard<SpinLock> lk(queueLock);
            if( seen.find(st) != seen.end()) return;
            
            seen.insert(st);
            queue.push_back(st);
        }
        template<class T>
        inline void apply(T op){
            std::lock_guard<std::mutex> lk(_queueLock);
            copy();
            for (auto &&st_ptr : queue)
                op(st_ptr);
        }
        private:
            inline void copy(){
                std::lock_guard<SpinLock> lk(queueLock);
                _queue.resize(0);
                _seen.clear();
                _queue.insert(_queue.begin(), queue.begin(), queue.end());
                _seen.insert(seen.begin(),seen.end());
                queue.resize(0);
                seen.clear();
            }

            std::vector<const structured_term*> queue;
            std::unordered_set<const structured_term*> seen;
            std::vector<const structured_term*> _queue;
            std::unordered_set<const structured_term*> _seen;
            SpinLock queueLock;
            std::mutex _queueLock;
    };
} // namespace ares

#endif