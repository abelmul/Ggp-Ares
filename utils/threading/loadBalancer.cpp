#include "utils/threading/loadBalancer.hh" 
#include "utils/threading/threadPool.hh"
namespace ares
{
    LoadBalancer::LoadBalancer(ushort nWrks):nWorkers(nWrks),outstanding_work(0){
        /**
        * Each worker thread calls this to notify the loadbalancer
        * how many jobs it executed.
        */
        auto cb = [&](uint executed){
            {
                std::lock_guard<std::mutex> lk(mOutstdWork);
                outstanding_work-=executed;  
                if( outstanding_work != 0) return;
            }
            cvCheckEmpty.notify_one();
        };
        /* create the worker threads*/
        for(uint i=0; i < nWorkers ; i++)
            workerThreads.push_back(new WorkerThread(cb));
    }

    void LoadBalancer::shutdown(){
        for(auto &&wth : workerThreads ){
            wth->join();
            delete wth;
        }
    }

    /**
     * Assigns the @param job to one of the worker threads.
     * Inorder to balance the load submit jobs in round robin fashion.
     */
    void LoadBalancerRR::assign(JobQueue::Job_t job){
        static uint curr =0;  
        {
            std::lock_guard<SpinLock> lk(slk);
            curr = current;
            current = (current+1) % nWorkers;
        }
        WorkerThread* wth = workerThreads[curr];
        wth->submit(job);
    }
}