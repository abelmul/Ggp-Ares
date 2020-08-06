#ifndef BODY_HH
#define BODY_HH
#include <vector>
#include "utils/utils/exceptions.hh"
#include <initializer_list>
#include "utils/threading/locks.hh"
#include <mutex>
#include "utils/memory/memoryPool.hh"

namespace ares
{
    /**
     * This is just a wrapper class for a std::vector<T*>.
     * This class provides an efficient and suitable enviroment 
     * to reuse Literal, Function, and Clause Bodies (by using a MemoryPool).
     * This is to avoid calling operator new() each time a Body is created.
     */
    template<class T>
    struct _Body
    {
        typedef const typename std::vector<std::shared_ptr<T>>::const_iterator const_vec_iterator;
        private:
            /**
             * This is called by GdlParser, to first create a
             * structured_term without knowing its arity.
             */
            _Body(){
                _front = 0;
                container = new std::vector<std::shared_ptr<T>>();
            }
            /**
             * Same as _Body _Body(const_vec_iterator& begin, const_vec_iterator& end) 
             * but with no arity check for zero.
             */
            _Body(const_vec_iterator& begin, const_vec_iterator& end,bool):_Body(){
                container->insert(container->begin(), begin, end);
            }
            void push_back(std::shared_ptr<T> t){
                if( (const lit_container*)container == MemoryPool::EMPTY_CONTAINER){
                    int i;
                    std::cout << "tried to push to Body size 0, Stopping...\n";
                    std::cin >> i;
                }
                container->push_back(t);
            }

        public:
            _Body(const _Body&) = delete;
            _Body(const _Body&&) = delete;
            _Body& operator=(const _Body&) = delete;
            _Body& operator=(const _Body&&) = delete;

            _Body(arity_t _arity){
                _front = 0;
                //no need to allocate a container of arity 0 it should already be constructed by MemPool
                if( _arity == 0)
                    container = (std::vector<std::shared_ptr<T>>*) mempool->EMPTY_CONTAINER;
                else
                    container = (std::vector<std::shared_ptr<T>>*) mempool->allocate(_arity);
            }
            _Body(const_vec_iterator& begin, const_vec_iterator& end){
                _front = 0;
                arity_t _arity = end - begin;
                if( _arity == 0 )
                    container = (std::vector<std::shared_ptr<T>>*) mempool->EMPTY_CONTAINER;
                else{
                    container = (std::vector<std::shared_ptr<T>>*) mempool->allocate(_arity);
                    for (size_t i = 0; i < _arity; i++)
                        (*container)[i] = *(begin+i);
                } 
            }
            _Body(std::initializer_list<std::shared_ptr<T>> args){
                _front = 0;
                arity_t _arity = args.end() - args.begin();
                if( _arity == 0 )
                    container = (std::vector<std::shared_ptr<T>>*) mempool->EMPTY_CONTAINER;
                else{
                    container = (std::vector<std::shared_ptr<T>>*) mempool->allocate(_arity);
                    for (size_t i = 0; i < _arity; i++)
                        (*container)[i] = *(args.begin()+i);                    
                }
            }
            
            void* operator new(std::size_t){
                return mempool->allocate(body_pool_t);
            }

            void operator delete(void* p){
                mempool->deallocate((_Body<T>*)p);
            }

            ~_Body(){
                const auto& arity =  container->size();
                for (uint i=_front;i < arity;i++)
                    (*container)[i].reset();

                if(arity > 0)
                    mempool->deallocate(container);
                
                _front = 0;
                container = nullptr;
            }

            std::shared_ptr<T>& operator[](std::size_t i)const{
                i += _front;
                if(i >= container->size() ) 
                    throw IndexOutOfRange("Body size out of range size is : " + std::to_string(size()) + " ,index : " + std::to_string(i) + "\n");

                return (*container)[i];
            }
            const_vec_iterator begin()const{
                //the body begining could be offseted while poping
                return (container->cbegin() + _front);    
            }
            const_vec_iterator end()const{
                return container->cend();
            }
            void front_to_back(){
                if( this->size() <= 1 ) throw EmptyBodyPop("front_to_back called on a body of size <= 1.");

                const std::shared_ptr<T>  t = *this->begin();
                container->erase(this->begin());
                container->push_back(t);
            }
            void pop_front(){
                if( this->size() == 0 ) throw EmptyBodyPop("pop_front called on an empty body.");
                (*container)[_front].reset();
                _front++;
            }
            std::size_t size()const{ return container->size() - _front;}
        
    
            static MemoryPool* mempool;
        private:
            /**
             * Keep track of the front during pops rather than actually deleting the element
             * which makes the body not reusable.
             */
            std::size_t _front = 0;
            std::vector<std::shared_ptr<T>>* container = nullptr;
            
        friend class GdlParser;
        friend class Transformer;
        friend class MemoryPool;
    };

    
} // namespace ares

#endif