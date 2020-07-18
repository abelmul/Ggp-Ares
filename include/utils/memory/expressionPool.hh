#ifndef EXPRESSIONPOOL_HH
#define EXPRESSIONPOOL_HH

#include "utils/gdl/gdl.hh"
#include "utils/utils/hashing.hh"
#include "utils/memory/memoryPool.hh"
#include <boost/thread/pthread/shared_mutex.hpp>
#include "utils/memory/queue.hh"
#include <thread>

namespace ares
{

    /**
     * Responsible for the creation and deletion of expressions,
     * expressions are, names(char*) of predicates and terms, predicates and terms.
     */
    class ExpressionPool
    {

        typedef std::unordered_map<PoolKey, fn_sptr,PoolKeyHasher,PoolKeyEqual> FnPool;
        typedef std::unordered_map<PoolKey, lit_sptr,PoolKeyHasher,PoolKeyEqual> LitPool;
        typedef std::unordered_map<const char*, FnPool, CharpHasher,StrEq> NameFnMap;
        typedef std::unordered_map<const char*, LitPool,CharpHasher,StrEq> NameLitMap;
        friend class GdlParser;
    private:
        /**
         * Only a single expression pool should exist through out the life time of the application
         * and its created by the singleton gdlParser.
         */
        ExpressionPool(){
            for (const char* c : constants)
                litPool[c];         //Creates Empty LiteralPool 
            
            //Create the thread which polls the deletion queue
            pollDone = false;
            auto poll = [&](){
                remove(this->litQueue,litPool, this->litlock);
                remove(this->fnQueue,fnPool, this->fnlock);
            };
            queueThread = new std::thread(poll);
        }
        /**
         * The key exists in pool but the pool[key] (= shared_ptr) has been reseted so its assume gone.
         * Therefore reset it with a new object of type T.
         */
        template<class T>
        inline std::shared_ptr<T> reset(std::shared_ptr<T>& st_ptr, const char* name, PoolKey& key,boost::shared_mutex& lock){
            std::lock_guard<boost::shared_mutex> lk(lock); //Exclusive
            if(st_ptr.use_count() == 0 )
                st_ptr.reset(new T(name,key.p, key.body,&st_ptr));
            else{
                delete key.body;
            } 

            key.body = nullptr;
            std::shared_ptr<T> lr = st_ptr;
            return lr;
        }
        /**
         * The key does not exist in pool so might need to create it(if another thread han't already).
         * The name exists and is shared.
         */
        template<class T,class PT>
        inline std::shared_ptr<T> add(PT& pool, const char* name, PoolKey& key,boost::shared_mutex& lock){
            std::lock_guard<boost::shared_mutex> lk(lock); //Exclusive

            std::shared_ptr<T>& st_ptr = pool[key];         //THis gets the st_ptr or creates a new one.
            if(st_ptr.use_count() == 0 )
                st_ptr.reset(new T(name,key.p, key.body,&st_ptr));
            else
                delete key.body;

            key.body = nullptr;
            std::shared_ptr<T> lr = st_ptr;
            return lr;
        }
        /**
         * The key does not exist in pool so might need to create it(if another thread han't already).
         * The name doesn't exist.
         */
        template<class T,class PT>
        inline std::shared_ptr<T> add(PT& pool,PoolKey& key,boost::shared_mutex& lock){
            char* name = strdup(key.name);  //Create the name

            std::lock_guard<boost::shared_mutex> lk(lock); //Exclusive
            std::shared_ptr<T>& st_ptr = pool[name][key];       //THis gets the st_ptr or creates a new one.
            
            if(st_ptr.use_count() == 0 )
                st_ptr.reset(new T(name,key.p, key.body,&st_ptr));
            else{
                delete key.body;
                delete name;
            } 
            key.name = nullptr;
            key.body = nullptr;
            std::shared_ptr<T> lr = st_ptr;
            return lr;
        }
       /**
        * Poll the deletion queue and reset queued shared pointers
        * if the only reference that exists is within the pool.
        */
       template<class T,class TP>
       void remove(DeletionQueue<T>& queue,TP& pool, boost::shared_mutex& lock){
           while (!pollDone)
            {
                sleep(cfg.deletionPeriod);
                {
                    std::lock_guard<boost::shared_mutex> lk(lock);//Exclusively lock the pool
                    /* if use_count() == 1 then the only copy that exists is within the expression pool. So delete it.*/
                    auto _reset = [&](T st){
                        /* use_count could be > 1 b/c st could be reused between the time
                        * its queued for deletion and its actuall deletion. 
                        */
                        if( st->_this->use_count() > 1 ) return; 
                        auto it = pool.find(st->name);
                        if( it == pool.end() ) throw BadAllocation("Removing Function whose name is non existent in pool.");
                        PoolKey key{nullptr, st->_body, st->positive};
                        auto itspt = it->second.find(key);
                        if( itspt == it->second.end()) throw BadAllocation("Removing Function non existent in pool.");
                        it->second.erase(itspt);
                    };
                    queue.apply(_reset);
                }
            }
       }

    public:
        /**
         * The get* methods make sure only one instance of an object exists.
         * sets @param exists to true if the expression exists. if exists
         * They are all thread safe. 
         */
        cnst_var_sptr getVar(const char* var);
        cnst_const_sptr getConst(const char* c);
        /**
         * if the function is in the pool, then the existing function is returned. 
         * else a new function with name key.name and body key.body is created. 
         * key.body is now owned by ExpressionPool, and should neither be deleted nor
         * assigned to other objects at any point. This is to avoid copying the body while creation.
         */
        cnst_fn_sptr getFn(PoolKey& key);
        /**
         * if the literal is in the pool, then the existing literal is returned. 
         * else a new literal with name key.name and body key.body is created. 
         * key.body is now owned by ExpressionPool, and should neither be deleted nor
         * assigned to other objects at any point. This is to avoid copying the body while creation.
         */
        cnst_lit_sptr getLiteral(PoolKey& key);

        /**
         * reset the shared_ptr of st if use_count == 2 hence
         * the only copy that exists is only in the expr pool.
         */
        inline void remove(cnst_lit_sptr* lit){
            if( lit->use_count() > 2 ) return;
            if( lit->use_count() < 2 ) throw BadAllocation("Removing Literal non existent in pool.");
            //The references that exist are just this and in the pool, schedule deletion
            litQueue.enqueue(lit->get());
            lit->reset();
        }
        inline void remove(cnst_fn_sptr* fn){
            if(fn->use_count() > 2 ) return;
            if( fn->use_count() < 2 ) throw BadAllocation("Removing Function non existent in pool.");
            //The references that exist are just this and in the pool, schedule deletion
            fnQueue.enqueue(fn->get());
            fn->reset();
        }
        
        ~ExpressionPool(){
            queueThread->join();
        }

        const char* ROLE = "role";
        const char* INIT = "init";
        const char* LEGAL = "legal";
        const char* NEXT = "next";
        const char* TRUE = "true";
        const char* DOES = "does";
        const char* DISTINCT = "distinct";
        const char* GOAL = "goal";
        const char* TERMINAL = "terminal";
        const char* INPUT = "input";
        const char* BASE = "base";

    private:

        std::unordered_map<const char*, cnst_var_sptr,CharpHasher,StrEq> varPool;
        std::unordered_map<const char*, cnst_const_sptr,CharpHasher,StrEq> constPool;
        NameFnMap fnPool;
        NameLitMap litPool;

        DeletionQueue<const Function*> fnQueue;
        DeletionQueue<const Literal*> litQueue;

        std::thread* queueThread;
        bool pollDone;

        /** 
         * The below mutexes provide multiple(shared) readers/single(exclusive) writer access to 
         * their respective pools.
         */        
        boost::shared_mutex varlock;
        boost::shared_mutex constlock;
        boost::shared_mutex fnlock;
        boost::shared_mutex litlock;

        /**
         * Define the GDL specific(game independant) constants
         */

        const std::vector<const char *> constants{ ROLE,TRUE,INIT,NEXT,LEGAL,DOES,DISTINCT,GOAL,TERMINAL };
    };
} // namespace ares

#endif