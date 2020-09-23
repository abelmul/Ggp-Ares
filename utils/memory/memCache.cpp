#include "utils/memory/memCache.hh"
#include <chrono> 

namespace ares
{
    MemCache::~MemCache(){
        {
            std::unique_lock<std::mutex> lk(mRemove);
            pollDone = true;
        }
        cvRemove.notify_all();

        delQueueTh->join();

        delete delQueueTh;

        nameLitPool.clear();
        nameFnPool.clear();
        
        for( auto&& [name, varptr] : varPool){
            auto* v = varptr.get();
            varptr.reset();
            free((void*)v);
        }
        for( auto&& [name, cnstptr] : constPool){
            auto* c = cnstptr.get();
            cnstptr.reset();
            free((void*)c);
        }
    }
    cnst_var_sptr MemCache::getVar(ushort n){
        cnst_var_sptr vsptr;
        VarPool::accessor ac;
        if( varPool.insert(ac,n) ) //This thread inserted the key
            ac->second.reset(new Variable(n))/* reset(new Variable(n)); */;
        
        vsptr = ac->second;
        ac.release();
        return vsptr;
    }
    cnst_const_sptr MemCache::getConst(ushort n){
        cnst_const_sptr csptr;
        ConstPool::accessor ac;
        if( constPool.insert(ac,n) )    //This thread inserted the key
            ac->second.reset(new Constant(n));

        csptr = ac->second;
        ac.release();
        return csptr;
    }
    cnst_fn_sptr MemCache::getFn(PoolKey& key){
        NameFnMap::accessor ac;
        nameFnPool.insert(ac,key.name);     //insert it if it doesn't exist
        FnPool& fnPool= ac->second;
        FnPool::accessor fnAc;
        fn_sptr fn;
        if( fnPool.insert(fnAc, key) ){
            //key didn't exist, Create a shared ptr and insert that
            fn.reset(new Function(key.name,key.body),Deleter(this));
            fnAc->second = fn;
        }
        else{
            //key  exists but weak ptr might be 0.
            fn = fnAc->second.lock();
            if( not fn ){
                ((PoolKey*)&fnAc->first)->body = key.body;  //Reassign it to the new body
                fn.reset( new Function(key.name,key.body),Deleter(this));
                fnAc->second = fn;
            }
            else
                delete key.body;
        }
        fnAc.release();
        key.body = nullptr;
        return fn;
    }
    cnst_lit_sptr MemCache::getLiteral(PoolKey& key){
        NameLitMap::accessor ac;
        nameLitPool.insert(ac,key.name);     //insert it if it doesn't exist
        LitPool& litPool= ac->second;
        LitPool::accessor litAc;
        lit_sptr lit;
        if( litPool.insert(litAc, key) ){
            //key didn't exist, Create a shared ptr and insert that
            lit.reset(new Literal(key.name,key.p,key.body),Deleter(this));
            litAc->second = lit;
        }
        else{
            //key  exists but weak ptr might be 0.
            lit = litAc->second.lock();
            if( not lit ){
                ((PoolKey*)&litAc->first)->body = key.body; //Reassign it to the new body
                lit.reset( new Literal(key.name,key.p,key.body),Deleter(this));
                litAc->second = lit;
            }
            else
                delete key.body;
        }
        litAc.release();
        key.body = nullptr;
        return lit;
    }
} // namespace ares
