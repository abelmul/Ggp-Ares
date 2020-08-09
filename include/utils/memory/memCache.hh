#ifndef EXPRESSIONPOOL_HH
#define EXPRESSIONPOOL_HH

#include "utils/gdl/gdl.hh"
#include "utils/utils/hashing.hh"
#include "utils/memory/memoryPool.hh"
#include <boost/thread/pthread/shared_mutex.hpp>
#include "utils/memory/queue.hh"
#include "utils/memory/namer.hh"
#include <tbb/concurrent_hash_map.h>
#include <condition_variable>
#include <thread>
#include <boost/thread/thread.hpp>


namespace ares
{

    /**
     * Responsible for the creation and deletion of expressions,
     * expressions are, names(char*) of predicates and terms, predicates and terms.
     */
    class MemCache
    {

        typedef tbb::concurrent_hash_map<ushort, cnst_var_sptr> VarPool;
        typedef tbb::concurrent_hash_map<ushort, cnst_const_sptr> ConstPool;

        typedef tbb::concurrent_hash_map<PoolKey, fn_wkptr,PoolKeyHasher> FnPool;
        typedef tbb::concurrent_hash_map<PoolKey, lit_wkptr,PoolKeyHasher> LitPool;

        typedef tbb::concurrent_hash_map<ushort, FnPool> NameFnMap;
        typedef tbb::concurrent_hash_map<ushort, LitPool> NameLitMap;

        typedef std::chrono::seconds seconds;

        friend class MemoryPool;

    public:

        /**
         * The get* methods make sure only one instance of an object exists.
         * sets @param exists to true if the expression exists. if exists
         * They are all thread safe. 
         */
        cnst_var_sptr getVar(ushort var);
        cnst_const_sptr getConst(ushort c);
        /**
         * if the function is in the pool, then the existing function is returned. 
         * else a new function with name key.name and body key.body is created. 
         * key.body is now owned by MemCache, and should neither be deleted nor
         * assigned to other objects at any point. This is to avoid copying the body while creation.
         */
        cnst_fn_sptr getFn(PoolKey& key);
        /**
         * if the literal is in the pool, then the existing literal is returned. 
         * else a new literal with name key.name and body key.body is created. 
         * key.body is now owned by MemCache, and should neither be deleted nor
         * assigned to other objects at any point. This is to avoid copying the body while creation.
         */
        cnst_lit_sptr getLiteral(PoolKey& key);

        ~MemCache();

    private:

        template <class T>
        struct Deleter;
        /**
         * Only a single expression pool should exist through out the life time of the application
         * and its created by the singleton gdlParser.
         */
        MemCache(){            
            auto poll_fn=[&]{  remove(fnQueue,nameFnPool,FN,seconds(cfg.deletionPeriodFn)); };
            auto poll_lit=[&]{ remove(litQueue, nameLitPool,  LIT, seconds(cfg.deletionPeriodLit));};
            pollDone =false;
            litQueueTh = new std::thread(poll_lit);
            fnQueueTh = new std::thread(poll_fn);
        }
        template <class T>
        struct Deleter{
            Deleter(MemCache* exp):_this(exp){}
            void operator()(T* st){
                if (st->get_type() == FN)   _this->fnQueue.enqueue((const Function*)st);
                else if(st->get_type()==LIT) _this->litQueue.enqueue((const Literal*)st);
            }
            MemCache* _this =nullptr;
        };
       /**
        * Poll the deletion queue and reset queued shared pointers
        * if the only reference that exists is within the pool.
        */
       template<class T,class TP>
       void remove(DeletionQueue<T>& queue,TP& nameStMap,Type type,seconds period){
           /* if use_count() == 1 then the only copy that exists is within the expression pool. So delete it.*/
            auto _reset = [&](T st){
                /* use_count could be > 1 b/c st could be reused between the time
                * its queued for deletion and its actuall deletion. 
                */
                if( not st->body ) return;
                if( type != st->get_type() ){
                    if( type == FN ) litQueue.enqueue((const ares::Literal*)st);
                    else if( type == LIT ) fnQueue.enqueue((const ares::Function*)st);
                    return;
                }
                typename TP::accessor ac;
                if ( not nameStMap.find(ac, st->name) )  throw BadAllocation("Tried to delete an st whose name doesnt exist in pool.");
                typename TP::mapped_type& pool = ac->second;
                typename TP::mapped_type::accessor pAc;
                PoolKey key{st->name, st->body, st->positive, nullptr};
                
                if( not pool.find(pAc,key)) throw BadAllocation("Tried to delete an st that doesn't exist in pool");
                if( pAc->second.use_count() > 0 ) return;
                pool.erase(pAc);
                delete st;
            };

           while (!pollDone)
            {
                {
                    std::unique_lock<std::mutex> lk(mRemove);
                    cvRemove.wait_for(lk, period,[&](){ return pollDone;} );
                }
                queue.apply(_reset);
            }
       }

    private:

        VarPool varPool;
        ConstPool constPool;
        
        NameFnMap nameFnPool;
        NameLitMap nameLitPool;

        DeletionQueue<const Function*> fnQueue;
        DeletionQueue<const Literal*> litQueue;

        std::thread *litQueueTh,*fnQueueTh;

        std::condition_variable cvRemove;
        std::mutex mRemove;
        bool pollDone;
    };
} // namespace ares

#endif