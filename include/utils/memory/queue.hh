#ifndef QUEUE_HH
#define QUEUE_HH
#include "utils/gdl/gdl.hh"
#include <unordered_set>
#include <thread>
#include <functional>
#include <array>
#define BUCKET_SIZE 64

namespace ares
{
    typedef u_char byte;

    template<class T>
    struct DeletionQueue{
        // enum types {queue, lock};
        typedef std::function<void(T)> op_t;
        DeletionQueue(){
            queues[0] = queues[1] = UniqueVector<T>(cfg.deletionQueueSize);
            i=0;
        }

        inline void enqueue(T st){
            std::lock_guard<SpinLock> lk(lock);
            queues[i].push_back(st);
        }
        
        inline void apply(op_t op){
            byte j;
            {
                std::lock_guard<SpinLock> lk(lock);
                j = i;
                i = 1-i;
            }
            for (auto &&st : queues[j])
                op(st);
            
            queues[j].clear();
        }
        private:
            UniqueVector<T> queues[2];
            SpinLock lock;
            byte i;
    };
    // template<ushort _Nm=BUCKET_SIZE>
    struct JobQueue
    {
        typedef std::function<void(void)> Job_t;
        
        private : struct Node;
        public:
            JobQueue(){
                //remove from front, push to back
                front = new Node();
                front->next = new Node();
                back = front;
                end = back->next;
                outstanding_work=0;
                index = 0;
                size();
            }
            void enqueue(Job_t j){
                std::lock_guard<std::mutex> lkOuts(mOutstdWork);

                if( index == BUCKET_SIZE) grow();
                back->bucket[index] = j;
                index++;
                outstanding_work++;
            }

            uint poll(){
                Node *_front, *_back;
                uint executed =0;
                byte _index;
                while (true)
                {
                    {
                        std::lock_guard<std::mutex> lk(mOutstdWork);
                        if( noJob() ){
                            if( executed == 0) 
                                return 0;
    
                            outstanding_work -=executed;
                            end->next = _front;
                            end = _back;
                            return executed;
                        }
                        grow(_front,  _back, _index);
                    }
                    //traverse buckets starting from front
                    Node* n = _front;
                    while (n)
                    {
                        byte size = BUCKET_SIZE;
                        if( n == _back) size = _index;
                        
                        for( byte i =0; i< size; i++){
                            n->bucket[i]();
                            executed++;
                        }
                        n = n->next;
                    }
                }
                
            }
            bool empty() const {
                return outstanding_work == 0;
            }
            std::size_t size(){
                std::lock_guard<std::mutex> lk(mOutstdWork);
                uint i=1;
                Node* n =   front;
                while((n = n->next)) i++;
                return i;
            }
            std::mutex mOutstdWork;
        private:
            /**
             * shift back one node growing as needed.
             */
            inline Node* grow(){
                if( not back->next ){
                    back->next = new Node();
                    end = back->next;
                }

                back = back->next;
                index = 0;
                return back;
            }
            /**
             * shift back one node growing as needed but save front, back, and index in 
             * @param _front, @param _back, and @param _index;
             */
            inline void grow(Node*& _front, Node*&_back, byte& _index){
                _front = front;
                _back  = back;
                _index = index;
                front = grow();
                _back->next = nullptr;
            }

            inline bool noJob() const {
                return index == 0;
            }


            Node *front, *back,*end;
            std::size_t outstanding_work;
            byte index;

            struct Node
            {
                Node(){next = nullptr;}
                std::array<Job_t,BUCKET_SIZE> bucket;
                Node* next;
            };
    };
} // namespace ares

#endif