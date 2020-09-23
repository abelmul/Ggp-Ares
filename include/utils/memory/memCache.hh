#ifndef MEMCACHE_HH
#define MEMCACHE_HH

#include "utils/gdl/gdl.hh"
#include "utils/utils/hashing.hh"
#include "utils/memory/memoryPool.hh"
#include "utils/threading/threading.hh"
#include "utils/memory/queue.hh"
#include "utils/memory/namer.hh"

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
    //ctor
    private:
        /**
         * Only a single Memory cache should exist through out the life time of the application
         * and its created by the singleton MemoryPool.
         */
        MemCache(){            
            auto del_poll=[&]{  remove(seconds(cfg.deletionPeriod)); };
            pollDone =false;
            delQueueTh = new std::thread(del_poll);
        }

        MemCache(const MemCache&)=delete;
        MemCache& operator=(const MemCache&)=delete;
        MemCache(const MemCache&&)=delete;
        MemCache& operator=(const MemCache&&)=delete;
    /**
     * Methods
     */
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
        struct Deleter{
            Deleter(MemCache* exp):this_(exp){}
            void operator()(const structured_term* st){
                this_->delQueue.enqueue(st);
            }
            MemCache* this_ =nullptr;
        };
       /**
        * Poll the deletion queue and delte queued pointers
        * and also remove thier entries in the pool if another thread
        * hasn't allocated the entry a new pointer.
        */
       void remove(seconds period){
           auto reset = [&](const structured_term* st){
                if( st->get_type() == Term::FN )
                    remove_<NameFnMap,FnPool>(nameFnPool, st);
                else
                    remove_<NameLitMap,LitPool>(nameLitPool, st);
            };

           while (!pollDone)
            {
                {
                    std::unique_lock<std::mutex> lk(mRemove);
                    cvRemove.wait_for(lk, period,[&](){ return pollDone;} );
                }
                delQueue.apply(reset);
            }
            while (not delQueue.empty() )
                delQueue.apply(reset);
        }
        /** 
         * if use_count() == 0 then the weak_ptr that exists is within the expression pool has expited.
         * So delete the entry.
         */
        template<class T,class TP>
        inline void remove_(T& namePool, const structured_term* st){
            typename T::accessor ac;
            if( !namePool.find(ac,st->get_name()) ) throw BadAllocation("Tried to delete an st with name that doesn't exist");
            PoolKey key{st->get_name(), &st->getBody(), bool(*st)};
            TP& pool = ac->second;
            typename TP::accessor pAc;
            //This entry has been reassigned and then may deleted later
            if( !pool.find(pAc, key) or (pAc->second.lock()) ){delete st; return;};
            pool.erase(pAc);    //Can safley erase this entry, as it hasn't been reassigned.
            delete st;
        }

    /**
     * Data
     */
    private:

        VarPool varPool;
        ConstPool constPool;
        
        NameFnMap nameFnPool;
        NameLitMap nameLitPool;

        DeletionQueue<const structured_term*> delQueue;

        std::thread *delQueueTh;

        std::condition_variable cvRemove;
        std::mutex mRemove;
        bool pollDone;
    };
} // namespace ares

#endif