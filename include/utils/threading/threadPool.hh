#ifndef THREADPOOL_HH
#define THREADPOOL_HH

#include <mutex>
#include <condition_variable>
#include <boost/asio/io_service.hpp>
#include <thread>
#include <unordered_set>

namespace ares
{
    class ThreadPool
    {
    protected:

        /**
         * The worker threads execution loop.
         */
        void workerThread();
        /**
         * The calling thread is valid iff it can schedule a job
         * iff One of the following conditions is met
         * 1. The calling thread is owning thread. or
         * 2. The calling thread is one of the worker threads. 
         * and
         * 1. The pool is currently not stopped. and
         * 2. The pool is currently owned.
         */
        bool validThread();
        /**
         * The 'virtual' work queue is empty.
         */ 
        bool queueEmpty(){ return outstanding_work == 0; }

        boost::asio::io_service ioService;
        boost::asio::io_service::work* work;
        std::thread* workT;
        std::vector<std::thread*> threads;

        /**
         * One thread pool is only used for an exploration of a single SLD-tree at a time
         * This is because we want to guarantee this :- all the worker threads are free iff the
         * whole sld-tree has been explored (for a single tree). So if we use the thread pool
         * to explore multiple sld trees (that is used to prove different queries) at the same 
         * time the thread would be busy until both sld-trees are explored. So we wouldn't know
         * when one is proven. We would have to wait until both are proven. Which might even create 
         * deadlocks if used to prove negative literals while proving the original query!
         */
        std::thread::id owner;
        std::unordered_set<std::thread::id> workersId;

        ushort nWorkers;                        //Number of worker threads.
        uint outstanding_work = 0;          //are there any remaning jobs posted by the current owner.
        

        bool owned = false;                     //is member field this->owner valid?
        bool stopped = false;                   //are we accepting jobs from current owner?
        bool finished = false;                  //Workers constantly check this to know if they should exit processing loop.

        std::mutex mOutstdWork;
        std::mutex mOwner;

        std::condition_variable cvOutstdWork;
        std::condition_variable cvOwner;
        std::condition_variable cvCheckEmpty;
        

        // const static std::thread::id NO_OWNER = NULL;

    public:
        ThreadPool(ushort workers): nWorkers(workers){
            //Initialize the worker threads;
            std::thread* t;
            workT = new std::thread( [this](){
                this->work = new boost::asio::io_service::work(this->ioService);
            });

            for (size_t i = 0; i < workers; i++)
            {
                t = new std::thread(std::bind(&ThreadPool::workerThread,this) );
                threads.push_back(t);
                workersId.insert(t->get_id());
            }   
        }

        /**
         * Schedule a task. T should be callable!
         * @returns false if the calling thread is not the owner (or one of the delegated worker threads).
         */
        template <class T>
        bool post(T job);
        /**
         * Wait untill all submitted jobs by the owner thread are executed.
         * Returns false immediatley if calling thread != owner.
         * After this call returns the thread pool is not owned by any thread.
         */
        bool wait();
        /**
         * If called by the owning thread, then  no more jobs are scheduled after this point, untill ownership is re-acquired.
         * It waits for all jobs, submitted before this point, to finish.
         * @returns true iff called by the owning thread (or one of the delegated worker threads).
         */
        bool stop();
        /**
         * Blocks untill exclusive ownership of the pool is acquired. 
         * @returns the calling threads id.
         */
        std::thread::id acquire();
        /**
         * A non blocking version of acquire.
         * @returns true if the pool is acquired, false otherwise.
         */
        bool try_acquire();
        
        ~ThreadPool() {
            delete work;
            workT->join();
            delete workT;
            ioService.stop();
            finished = true;
            //if any threads are waiting unblock them
            {
                std::unique_lock<std::mutex> lk(mOutstdWork);
                outstanding_work = 1;    
            }
            cvOutstdWork.notify_all();
            //by now every thread is unblocked and out of their processing loop.
            for (std::thread* t : threads)
            {
                t->join();
                delete t;
            }
            
        }
        friend class TestableThreadPool;
    };

    template <class T>
    bool ThreadPool::post(T job){
        //Only the owning thread or one of the delegated worker threads should submit
        //and pool musn't be stopped and owned.
        if( !validThread() ) return false;

        ioService.post(job);

        {
            std::unique_lock<std::mutex> lck(mOutstdWork);
            outstanding_work++;
        }
        cvOutstdWork.notify_one();
        return true;
    }
} // namespace ares


#endif