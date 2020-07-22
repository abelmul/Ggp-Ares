#ifndef LOAD_BALANCER_HH
#define LOAD_BALANCER_HH

#include <mutex>
#include "utils/memory/queue.hh"
#include <condition_variable>
#include <vector>
#include <functional>
#include <atomic>

namespace ares
{
    class ThreadPool;
    class WorkerThread;
    /**
    * This is the interface all LoadBalancers will have.
    */
    class LoadBalancer{
        public:
            std::condition_variable cvCheckEmpty;
            std::mutex mOutstdWork;

            LoadBalancer(ushort nWrks);
            /**
             *  execute @param job
             */
            inline void submit(JobQueue::Job_t job){
                {
                    std::lock_guard<std::mutex> lk(mOutstdWork);
                    outstanding_work++;
                }
                assign(job);
            }

            /**
            * All submitted jobs are executed.
            */
            bool noJobs(){return outstanding_work == 0;}
            /**
            * destructs all the the running worker threads.
            */
            void shutdown();
            virtual ~LoadBalancer(){}
        protected:
            uint nWorkers;
            std::vector<WorkerThread*> workerThreads;
            std::size_t outstanding_work; //total outstanding work

            /**
             * Assign this @param job to one of the worker threads.
             */
            virtual void assign(JobQueue::Job_t job) = 0;
    };


    /**
    * submits jobs to the threads in a round robin fashion.
    */
    class LoadBalancerRR : public LoadBalancer{
        public:
            LoadBalancerRR(ushort nWorkers):LoadBalancer(nWorkers),current(0){}
        protected:
            /**
            * Assigns the @param job to one of the worker threads.
            * Inorder to balance the load submit jobs in round robin fashion.
            */
            virtual void assign(JobQueue::Job_t job);


        private:
            SpinLock slk;
            ushort current;
    };
}
#endif