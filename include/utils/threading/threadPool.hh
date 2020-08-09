#ifndef THREADPOOL_HH
#define THREADPOOL_HH

#include <mutex>
#include <condition_variable>
#include <thread>
#include <unordered_set>
#include <atomic>
#include <condition_variable>
#include "utils/threading/loadBalancer.hh"
#include "utils/threading/callbacks.hh"
namespace ares
{
    class ThreadPool
    {
    protected:
        LoadBalancer* loadBalancer;
        std::atomic<bool> stopped = false;                   //are we accepting jobs from current owner?
        bool finished = false;                  //Workers constantly check this to know if they should exit processing loop.

    public:
        ThreadPool(LoadBalancer* ldB):loadBalancer(ldB){}

        /**
         * Schedule a job.
         */
        inline void post(JobQueue::Job_t job){
            //queue the job
            if( stopped.load() ) return;
            
            loadBalancer->submit(job);
        }
        /**
         * Wait untill all submitted jobs are executed.
         */
        inline void wait(){
            std::unique_lock<std::mutex> lk(loadBalancer->mOutstdWork);
            loadBalancer->cvCheckEmpty.wait(lk, [this](){ return loadBalancer->noJobs(); });
            stop();
        }
        /**
         * No more jobs are scheduled after this point, untill pool is re-started using restart().
         */
        inline void stop(){
            stopped.store(true);
        }
        /**
         * Start accepting jobs.
         */
        inline void restart(){
            stopped.store(false);
        }
        ~ThreadPool(){
            loadBalancer->shutdown();
            delete loadBalancer;
        }
        friend class TestableThreadPool;
    };

    /**
    * This class encapsulates the idea of a worker thread.
    * each worker thread has 1 job queue  1 thread  which it
    * accepts and executes a job on.
    */
    struct WorkerThread{
        typedef std::function<void(uint)> cb_t;
        WorkerThread(cb_t c):cb(c)
        {
            finished = false;
            auto f = std::bind(&WorkerThread::operator(), this);
            thread = new std::thread(f);
        }
        WorkerThread(const WorkerThread&)=delete;
        WorkerThread(const WorkerThread&&)=delete;
        WorkerThread& operator=(const WorkerThread&)=delete;
        WorkerThread& operator=(const WorkerThread&&)=delete;
        /**
            * The worker threads' processing loop.
            */
        void operator()(){
            while (!finished)
            {
                {
                    std::unique_lock<std::mutex> lck(jobs.mOutstdWork);
                    cvOutstdWork.wait(lck, [this](){
                        return (!jobs.empty()) or finished;
                    });
                    if( finished ) return;
                }

                uint executed = jobs.poll();
                cb(executed);
            }
        }
        /**
            * submit a job for this worker thread. Does all the job present
            * in its jobqueue and @returns the number jobs executed.
            */
        inline void submit(JobQueue::Job_t job){
            jobs.enqueue(job);
            cvOutstdWork.notify_one();
        }
        inline void join(){ 
            {
                std::unique_lock<std::mutex> lck(jobs.mOutstdWork);
                finished = true;
            }
            cvOutstdWork.notify_one();
            thread->join(); 
            delete thread; 
        }
        private:
            cb_t cb;        //Is called evertime this worker executes a job.
            bool finished;
            
            JobQueue jobs;
            std::condition_variable cvOutstdWork;
            std::thread* thread;
    };
} // namespace ares


#endif