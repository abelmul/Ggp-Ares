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


        typedef std::unordered_map<PoolKey, fn_wkptr,PoolKeyHasher,PoolKeyEqual> FnPool;
        typedef std::unordered_map<PoolKey, lit_wkptr,PoolKeyHasher,PoolKeyEqual> LitPool;
        typedef std::unordered_map<const char*, FnPool, CharpHasher,StrEq> NameFnMap;
        typedef std::unordered_map<const char*, LitPool,CharpHasher,StrEq> NameLitMap;
        friend class GdlParser;
    public:
        struct timer{
            timer(ExpressionPool* _t):_this(_t){
                begin = _this->crono.now();
            }
            ~timer(){
                end = _this->crono.now();
                std::lock_guard<SpinLock> lk(_this->ts);
                _this->time_spent += std::chrono::duration_cast<std::chrono::microseconds>(end - begin).count();
            }
            ExpressionPool* _this;
            std::chrono::high_resolution_clock::time_point begin;
            std::chrono::high_resolution_clock::time_point end;
        };
    private:
        std::chrono::high_resolution_clock crono;
        template <class T>
        struct Deleter;
        /**
         * Only a single expression pool should exist through out the life time of the application
         * and its created by the singleton gdlParser.
         */
        ExpressionPool(){
            time_spent = 0.0;
            for (const char* c : constants)
                litPool[c];         //Creates Empty LiteralPool 
            
            auto poll_fn=[&]{ remove(fnQueue,fnPool,fnlock,FN); };
            auto poll_lit=[&]{ remove(litQueue, litPool, litlock, LIT);};
            litQueueTh = new std::thread(poll_lit);
            fnQueueTh = new std::thread(poll_fn);
        }
        /**
         * The key exists in pool but the pool[key] (= shared_ptr) has been reseted so its assume gone.
         * Therefore reset it with a new object of type T.
         */
        template<class T,class PT>
        inline std::shared_ptr<T> reset(PT& pool, const char* name, PoolKey& key,boost::shared_mutex& lock,bool doLock=true){
            if (doLock) lock.lock();    //Exclusive
            std::shared_ptr<T> lr;
            //Check if its been erased
            auto it = pool.find(key);
            if( it == pool.end() )      //Its been erased before exclusive ownership could be obained.
            {
                key._this =  new T(name,key.p, key.body);
                lr.reset((T*)key._this,Deleter<T*>(this));  //Create a new entry with key
                pool[key] = lr;
            }
            else        //Reallocation after the structured term is queued for deletion.
            {
                lr = pool[key].lock();
                if(not lr){
                    lr.reset((T*)it->first._this,Deleter<T*>(this));                   //Just reuse the previous value
                    pool[key] = lr;    //check if another thread has added it.
                }

                delete key.body;
            }
            key.body = nullptr;
            if (doLock) lock.unlock(); //Exclusive
            return lr;
        }
        /**
         * The key does not exist in pool so might need to create it(if another thread han't already).
         * The name doesn't exist.
         */
        template<class T,class PT>
        inline std::shared_ptr<T> add(PT& pool,PoolKey& key,boost::shared_mutex& lock){
            char* name = strdup(key.name);  //Create the name
            key.name = nullptr;
            std::lock_guard<boost::shared_mutex> lk(lock); //Exclusive
            std::shared_ptr<T> stp;
            if( pool.find(name) != pool.end()){
                //some other thread has inserted.
                stp = reset<T>(pool[name], name, key,lock,false);
                delete name;
            }
            else
                stp = reset<T>(pool[name], name, key,lock,false);
            
            return stp;
        }
        template <class T>
        struct Deleter{
            Deleter(ExpressionPool* exp):_this(exp){}
            void operator()(T st){
                if (st->get_type() == FN)   _this->fnQueue.enqueue((const Function*)st);
                else if(st->get_type()==LIT) _this->litQueue.enqueue((const Literal*)st);
            }
            ExpressionPool* _this =nullptr;
        };
       /**
        * Poll the deletion queue and reset queued shared pointers
        * if the only reference that exists is within the pool.
        */
       template<class T,class TP>
       void remove(DeletionQueue<T>& queue,TP& pool, boost::shared_mutex& lock,Type type){
           while (!pollDone)
            {
                sleep(cfg.deletionPeriod);
                {
                    timer t(this); 
                    std::lock_guard<boost::shared_mutex> lk(lock);//Exclusively lock the pool
                    /* if use_count() == 1 then the only copy that exists is within the expression pool. So delete it.*/
                    auto _reset = [&](T st){
                        /* use_count could be > 1 b/c st could be reused between the time
                        * its queued for deletion and its actuall deletion. 
                        */
                        if( not st->name ) return;
                        if( type != st->get_type() ){
                            if( type == FN ) litQueue.enqueue((const ares::Literal*)st);
                            else if( type == LIT ) fnQueue.enqueue((const ares::Function*)st);
                            return;
                        }
                        auto it = pool.find(st->name);
                        if( it == pool.end() ) throw BadAllocation("Removing Structured Term whose name is non existent in pool.");
                        PoolKey key{nullptr, st->_body, st->positive};
                        auto itspt = it->second.find(key);
                        if( itspt == it->second.end()) throw BadAllocation("Removing Structured Term non existent in pool.");
                        if( itspt->second.use_count() > 0 ) return;
                        it->second.erase(itspt);
                        delete st;
                    };
                    queue.apply(_reset);
                }
            }
       }

    public:
        double  time_spent;
        SpinLock ts;
        
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
 
        ~ExpressionPool(){
            pollDone = true;
            litQueueTh->join();
            fnQueueTh->join();
            delete litQueueTh;
            delete fnQueueTh;
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

        std::thread *litQueueTh,*fnQueueTh;
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