#include "utils/memory/memCache.hh"
#include <chrono> 

namespace ares
{
    MemCache::~MemCache(){}

    const Variable* MemCache::getVar(ushort n){
        VarPool::accessor ac;
        if( varPool.insert(ac,n) ) //This thread inserted the key
            ac->second.reset(new Variable(n),[&](Variable* v){deleter(v);});
        
        auto var = ac->second.get();
        ac.release();
        return var;
    }
    const Constant* MemCache::getConst(ushort n){
        ConstPool::accessor ac;
        if( constPool.insert(ac,n) )    //This thread inserted the key
            ac->second.reset(new Constant(n),[&](Constant* c){deleter(c);});

        auto cnst = ac->second.get();
        ac.release();
        return cnst;
    }
    const Function* MemCache::getFn(PoolKey& key){
        NameFnMap::const_accessor ac;
        nameFnPool.insert(ac,key.name);     //insert it if it doesn't exist
        FnPool* fnPool= (FnPool*)&ac->second;
        FnPool::accessor fnAc;
        const Function* fn;
        if( fnPool->insert(fnAc, key) ) //key didn't exist,so insert it
            fnAc->second.reset(new Function(key.name,key.body),[&](Function * f){deleter(f);});

        
        else //key  exists 
            delete key.body; 
        
        fn = fnAc->second.get();
        fnAc.release();
        key.body = nullptr;
        return fn;
    }
    const Atom* MemCache::getAtom(PoolKey& key){
        NameLitMap::const_accessor ac;
        nameLitPool.insert(ac,key.name);     //insert it if it doesn't exist
        LitPool* litPool= (LitPool*)&ac->second;
        LitPool::accessor litAc;
        const Atom* lit;
        if( litPool->insert(litAc, key) )//key didn't exist, insert it
            litAc->second.reset( new Atom(key.name,key.body),[&](Atom* l){deleter(l);});
        
        else //key  exists.
            delete key.body;
        
        lit = litAc->second.get();
        litAc.release();
        key.body = nullptr;
        return lit;
    }
    const Or* MemCache::getOr(PoolKey& key){
        OrPool::accessor orAc;
        const Or* or_;
        if( orPool.insert(orAc, key) )//key didn't exist, insert it
            orAc->second.reset( new Or(key.body),[&](Or* l){deleter(l);});
        
        else //key  exists.
            delete key.body;
        
        or_ = orAc->second.get();
        orAc.release();
        key.body = nullptr;
        return or_;
    }
    const Not* MemCache::getNot(PoolKey& key){
        NotPool::accessor notAc;
        const Not* not_;
        if( notPool.insert(notAc, key) )//key didn't exist, insert it
            notAc->second.reset( new Not(key.body),[&](Not* l){deleter(l);});
        
        else //key  exists.
            delete key.body;
        
        not_ = notAc->second.get();
        notAc.release();
        key.body = nullptr;
        return not_;
    }
} // namespace ares
