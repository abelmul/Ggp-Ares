#include "utils/gdl/gdlParser/expressionPool.hh"

namespace Ares
{
    const Variable* ExpressionPool::getVar(const char * n){
        varlock.lock_shared();
        if( varPool.find(n) != varPool.end()){
            Variable* v = varPool[n];
            varlock.unlock_shared();
            return v;
        }
        varlock.unlock_shared();
        //Exclusive ownership for writing
        varlock.lock();
        auto* v = new Variable(strdup(n));
        varPool[n] = v;
        varlock.unlock();
        return v;
    }
    const Constant* ExpressionPool::getConst(const char* c){
        constlock.lock_shared();
        if( constPool.find(c) != constPool.end() ){
            auto * cc = constPool[c];
            constlock.unlock_shared();
            return cc;
        }
        constlock.unlock_shared();
        //upgrad lock to exclusive
        constlock.lock();
        auto* cc = new Constant(strdup(c));
        constPool[c] = cc;
        constlock.unlock();
        return cc;
    }
    const Function* ExpressionPool::getFn(PoolKey& key, bool& exists){
        exists = false;
        fnlock.lock_shared();
        auto it = fnPool.find(key.name);
        if( it != fnPool.end() ){
            Function* fn;
            //function name exists!
            FnPool& fp = it->second;
            auto itp = fp.find(key);
            if( itp != fp.end() ){
                //function exists!
                exists = true;
                fn = itp->second;
                fnlock.unlock_shared();
                return fn;
            }
            else{
                fnlock.unlock_shared();
                fnlock.lock();  //Exclusive
                //Doesn't exist
                //share name i.e, it->first
                fn = new Function(it->first,key.body);
                fn->pool = this;
                fp[key] = fn;
                fnlock.unlock();  //Exclusive
                return fn;
            }
        }
        fnlock.unlock_shared();
        //Both the name and the do not exist
        fnlock.lock();  //Exclusive
        auto * name = strdup(key.name);
        auto* fn = new Function(name, key.body);
        fn->pool = this;
        fnPool[name][key] = fn;
        fnlock.unlock();  //Exclusive
        return fn;
    }
    const Literal* ExpressionPool::getLiteral(PoolKey& key, bool& exists){
        exists = false;
        litlock.lock_shared();
        auto it = litPool.find(key.name);
        if( it != litPool.end()){
            //Literal name exists
            Literal* l;
            auto& lp = it->second;
            auto itp = lp.find(key);
            if( itp != lp.end() ){
                //Literal exists
                exists = true;
                l = itp->second;
                litlock.unlock_shared();
                return l;
            }
            else{
                litlock.unlock_shared();
                litlock.lock(); //Exclusive
                l = new Literal(it->first,key.p, key.body);
                l->pool = this;
                lp[key] = l;
                litlock.unlock(); //Exclusive
                return l;
            }
        }
        litlock.unlock_shared();
        //Both name and body do not exist
        litlock.lock(); //Exclusive
        auto* name = strdup(key.name);
        auto* l = new Literal(name, key.p,key.body);
        l->pool = this;
        litPool[name][key] = l;
        litlock.unlock(); //Exclusive
        return l;
    }
} // namespace Ares
