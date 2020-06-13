#include "utils/threading/threadPool.hh"

namespace Ares
{
    void ThreadPool::workerThread(){
        while (!finished)
        {
            {
                std::unique_lock<std::mutex> lck(mOutstdWork);
                cvOutstdWork.wait(lck, [this](){
                    return !this->queueEmpty();
                });
                if( finished ) return;
            }

            {
                auto nHandlers = ioService.poll();      
                std::unique_lock<std::mutex> lck(mOutstdWork);
                outstanding_work -= nHandlers;                  //n handlers have been executed.
            }
            cvCheckEmpty.notify_one();   //Used to notify wait to check if queue is empty
        }
    }
    
    bool ThreadPool::validThread(){
        std::thread::id id = std::this_thread::get_id();
        std::unique_lock<std::mutex> lk(mOwner);
        auto valid = ( id == owner ) or ( workersId.find(id) != workersId.end() );
        return ( valid and owned and !stopped);
    }


    bool ThreadPool::wait() {
        std::thread::id id = std::this_thread::get_id();
        {
            std::unique_lock<std::mutex> lk(mOwner);
            if( (owner != id) or !owned ) return false;
        }

        std::unique_lock<std::mutex> lk(mOutstdWork);
        cvCheckEmpty.wait(lk, [this](){ return this->queueEmpty(); });
        
        {
            //At this point all submmitted jobs are executed, No more job will be posted using the owning thread!
            std::unique_lock<std::mutex> lk2(mOwner);
            owned = false;
        }

        cvOwner.notify_all();       //Notify that the thread pool could be reacquired;
        return true;
    }
    bool ThreadPool::stop(){
        if( !validThread() ) return false;
        std::unique_lock<std::mutex> lk(mOwner);
        stopped = true;         //Pool Won't accept new jobs after this point, until ownership is re-acquired.
        return true;
    }
    std::thread::id ThreadPool::acquire(){
        std::unique_lock<std::mutex> lk(mOwner);
        if( owned && std::this_thread::get_id() == owner) return owner;         //Called acquire while owning the pool!
        cvOwner.wait(lk, [this](){return not this->owned;} );
        owned = true;
        stopped = false;            //So that the new owner can submitt jobs.
        atleastOne = false;         //No jobs posted yet.
        owner = std::this_thread::get_id(); 
        return owner;
    }
    
    bool ThreadPool::try_acquire(){
        std::unique_lock<std::mutex> lk(mOwner);
        if( owned ){
            if(std::this_thread::get_id() == owner)
                return true;
            else
                return false;
        }

        owned = true;
        atleastOne = false;
        owner = std::this_thread::get_id(); 
        return true;
    }
} // namespace Ares
