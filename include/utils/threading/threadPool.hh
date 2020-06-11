#ifndef THREADPOOL_HH
#define THREADPOOL_HH

#include <mutex>
#include <condition_variable>
#include <boost/asio/io_service.hpp>
#include <boost/bind.hpp>
#include <boost/thread/thread.hpp>
#include <thread>
#include <unordered_set>
#include <iostream>

namespace Ares
{
    class ThreadPool
    {
    private:

        void workerThread();
        bool validThread();

        boost::asio::io_service::work* work;
        std::thread* workT;
        std::vector<std::thread*> threads;

        /**
         * One thread pool is only used for an exploration of a single SLD-tree at a time
         * This is because we want to guarantee this : all the worker threads free iff the
         * whole sld-tree has been explored (for a single tree). So if we use the thread pool
         * to explore multiple sld trees (that is used to prove different queries) at the same 
         * time the thread would be busy until both sld-trees are explored. So we wouldn't know
         * when one is proven. We would have to wait until both are proven. Which might even create 
         * deadlocks!
         */
        std::thread::id owner;
        std::unordered_set<std::thread::id> workersId;

        ushort nWorkers;                        //Number of worker threads.
        ushort freeWorkers;                     //Workers not doing anything.
        

        bool outstanding_work = false;          //is there any jobs posted by the current owner.
        bool owned = false;                     //is member owner valid?
        bool atleastOne = false;                //atleat on job is submitted by the current owner.
        bool finished = false;                  //Workers constantly check this to know when if they should stop.

        std::mutex mOutstdWork;
        std::mutex mOwner;
        std::mutex mFreeWrkrs;

        std::condition_variable cvOutstdWork;
        std::condition_variable cvOwner;
        std::condition_variable cvFreeWrkrs;
        

        // const static std::thread::id NO_OWNER = NULL;

    public:
    boost::asio::io_service ioService;
        ThreadPool(ushort workers): nWorkers(workers), freeWorkers(workers){
            //Initialize the worker threads;
            std::thread* t;
            workT = new std::thread( [this](){
                this->work = new boost::asio::io_service::work(this->ioService);
            });

            for (size_t i = 0; i < workers; i++)
            {
                t = new std::thread(boost::bind(&ThreadPool::workerThread,this) );
                threads.push_back(t);
                workersId.insert(t->get_id());
                // std::cout << "Created thread : " << i << std::endl;
            }   
            // std::cout << "Done Creating threads : " << std::endl;
        }

        /**
         * Schedule a task. T should be callable!
         * @returns false if the calling thread is not the owner.
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
         * Blocks untill exclusive ownership of the pool is acquired. 
         * @returns the calling threads id.
         */
        std::thread::id acquire();

        bool allFree(){
            return freeWorkers == nWorkers;
        }
        ~ThreadPool() {
            delete work;
            workT->join();
            delete workT;
            ioService.stop();
            finished = true;
            //if any threads are waiting unblock them
            {
                std::unique_lock<std::mutex> lk(mOutstdWork);
                outstanding_work = true;    
            }
            cvOutstdWork.notify_all();
            //by now every thread is unblocked and out of their processing loop.
            for (std::thread* t : threads)
            {
                t->join();
                delete t;
            }
            
        }
    };

    template <class T>
    bool ThreadPool::post(T job){
        //Only the owning thread or the worker thread should submit
        if( !validThread() ) return false;

        ioService.post(job);

        {
            std::unique_lock<std::mutex> lck(mOutstdWork);
            outstanding_work = true;
        }
        cvOutstdWork.notify_one();
        return true;
    }
} // namespace Ares


#endif