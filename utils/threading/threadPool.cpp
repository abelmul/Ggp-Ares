#include "utils/threading/threadPool.hh"

namespace Ares
{
    void ThreadPool::workerThread(){
        while (!finished)
        {
            {
                std::unique_lock<std::mutex> lck(mOutstdWork);
                cvOutstdWork.wait(lck, [this](){
                    return this->outstanding_work;
                });
                if(!finished ) outstanding_work = false;
            }
            atleastOne = true;

            {
                std::unique_lock<std::mutex> lck(mFreeWrkrs);
                --freeWorkers;              //This worker is not free anymore.    
            }

            ioService.poll();            //Check if there's any jobs.

            {
                std::unique_lock<std::mutex> lck(mFreeWrkrs);
                ++freeWorkers;              //This worker is now free.    
            }
            cvFreeWrkrs.notify_one();   //Used to check if all workers are free.
        }
    }

    bool ThreadPool::validThread(){
        std::thread::id id = std::this_thread::get_id();
        std::unique_lock<std::mutex> lk(mOwner);
        auto valid = ( id == owner ) or ( workersId.find(id) != workersId.end() );
        return ( valid and owned );
    }


    bool ThreadPool::wait(){
         std::thread::id id = std::this_thread::get_id();
        {
            std::unique_lock<std::mutex> lk(mOwner);
            if( (owner != id) or !owned ) return false;
        }

        std::unique_lock<std::mutex> lk(mFreeWrkrs);
        cvFreeWrkrs.wait(lk, [this](){
            return atleastOne and this->allFree();
        });
        {
            //At this point all workers are free, No more job will be posted using the owning thread!
            std::unique_lock<std::mutex> lk2(mOwner);
            owned = false;
        }
        cvOwner.notify_all();       //Notify that the thread pool could be reacuired;
        return true;
    }

    std::thread::id ThreadPool::acquire(){
        std::unique_lock<std::mutex> lk(mOwner);
        if( owned && std::this_thread::get_id() == owner) return owner;         //Called acquire while owning the pool!
        cvOwner.wait(lk, [this](){return not this->owned;} );
        owned = true;
        atleastOne = false;
        owner = std::this_thread::get_id(); 
        return owner;
    }
} // namespace Ares
