#include "utils/threading/threadPool.hh"
#include <cstdlib>
#include <ctime>
#include <iostream>
#include "common.hh"

using namespace std;
using namespace ares;

uint JOBS;
uint SUBJOBS;


class TestableThreadPool : public ThreadPool
{
    mutex lk;
    public:
        TestableThreadPool(ushort workers):ThreadPool(workers)
        {
        }
        
        template <class T>
        bool wait(T cb)
        {//Just modify it to notify the TesterObject

            std::thread::id id = std::this_thread::get_id();
            {
                std::unique_lock<std::mutex> lk(mOwner);
                if( (owner != id) or !owned ) return false;
            }

            std::unique_lock<std::mutex> lk(jobs.mOutstdWork);
            cvCheckEmpty.wait(lk, [this](){ return jobs.empty(); });
            
            {
                //At this point all submmitted jobs are executed, No more job will be posted using the owning thread!
                std::unique_lock<std::mutex> lk2(mOwner);
                owned = false;
                cb();
            }
            cvOwner.notify_all();       //Notify that the thread pool could be reacquired;
            return true;
        }
};

struct Tester
{
    uint asserts =0;
    int currentJobId = -1;
    uint nSubJobs = 0;
    uint jobs = 0;
    thread::id _id;
    mutex lk;
    //Should be owned by 1 thread at a time
    void assertExclusiveOwnership(int job_id){
        lock_guard<mutex> l(lk);
        //At this point pool must not be owned!
        assert(currentJobId == -1);
        currentJobId = job_id;
        _id = this_thread::get_id();
        asserts++;
    }
    void assertExclusiveUse(int job_id){
        lock_guard<mutex> l(lk);
        assert(currentJobId == job_id );
        asserts++;
    }

    void releaseOwnership(int job_id){
        lock_guard<mutex> l(lk);
        //Only the owner is allowed to release ownership
        assert( _id == this_thread::get_id() and currentJobId == job_id);
        assert( nSubJobs == SUBJOBS);       //Have all handlers submitted via post been executed?
        currentJobId = -1;
        nSubJobs = 0;
        jobs++;
        asserts++;
    }
    void countSubjobs(int job_id){
        lock_guard<mutex> l(lk);
        assert( currentJobId == job_id );
        nSubJobs++;
    }
    void assertAllJobsExec(){
        assert( jobs == JOBS);          //Have all jobs(root level) been executed?
        cout << green << "ThreadPool Testing Successfull." << reset<< endl;
        cout <<  green <<  asserts << " Asserts."<< reset<< endl;
    }
};

Tester tester;
void job(pair<int , int> ip,TestableThreadPool& pool,std::mutex& m){
    tester.assertExclusiveUse(ip.first);        //Assert only one job is being executed!
    auto f = std::bind([ip, &m](){ 
        // sleep( rand() % 2 );
        // sleep(a);
        tester.assertExclusiveUse(ip.first);    //Assert only one job is being executed!
        tester.countSubjobs(ip.first);        
        std::unique_lock<std::mutex> l(m);
    });
    // cout << "Posted Job (" << ip.first << ", "<<ip.second<<")" << "\n";
    pool.post(f);
}

void try_acquire(TestableThreadPool& pool,int i, std::mutex& m){
    // static mutex
    auto id = pool.acquire();
    tester.assertExclusiveOwnership(i);         //Any thread that owns the pool asserts its the only one currently!

    assert( std::this_thread::get_id() == id);
    for (uint j = i*SUBJOBS; j < (i+1)*SUBJOBS; j++){
        auto f = std::bind(&job,make_pair(i,j), ref(pool), ref(m));
        pool.post(f);
    }
    auto f = std::bind(&Tester::releaseOwnership,ref(tester), i);
    pool.wait<decltype(f)>(f);
}
int main(int argc, char const *argv[])
{
    srand((unsigned) time(0));
    for (size_t j = 0; j < 10; j++)
    {
        tester.jobs= 0;
        tester.nSubJobs = 0;
        tester.currentJobId = -1;

        JOBS = (rand() % 100) + 100;
        SUBJOBS = (rand() % 400) + 200;
        uint poolSize = (rand() % 20) + 1;
        TestableThreadPool pool(poolSize);
        cout << "Testing with Pool of size : "<< poolSize << " Initial Jobs :" << JOBS << ", and "<<  "Submitted Handlers : " << SUBJOBS << endl;
        std::mutex m;
        vector<thread> threads;
        for (size_t i = 0; i < JOBS; i++)
        {
            // 10 threads competing to acquire ownership of the pool.
            auto f = std::bind(&try_acquire, ref(pool), i, ref(m));
            threads.push_back( thread(f));
        }

        for (auto &&t : threads)
            t.join();

        tester.assertAllJobsExec();
    }
    
    return 0;
}

