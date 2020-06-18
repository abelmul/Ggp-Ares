#include "utils/memory/expressionPool.hh"

namespace ares
{
    cnst_var_sptr ExpressionPool::getVar(const char * n){
        varlock.lock_shared();
        if( varPool.find(n) != varPool.end()){
            cnst_var_sptr v = varPool[n];
            varlock.unlock_shared();
            return v;
        }
        varlock.unlock_shared();
        //Exclusive ownership for writing
        std::lock_guard<boost::shared_mutex> lk(varlock);
        auto name = strdup(n);
        cnst_var_sptr& v = varPool[name];
        if( v.use_count() == 0)
            v.reset( new Variable(name,&v));
        else
            delete name;
        
        cnst_var_sptr vr = v;
        return vr;
    }
    cnst_const_sptr ExpressionPool::getConst(const char* c){
        constlock.lock_shared();
        if( constPool.find(c) != constPool.end() ){
            cnst_const_sptr cc = constPool[c];
            constlock.unlock_shared();
            return cc;
        }
        constlock.unlock_shared();
        //upgrade lock to exclusive
        std::lock_guard<boost::shared_mutex> lk(constlock);
        auto name = strdup(c);
        cnst_const_sptr& cc = constPool[name];
        if( cc.use_count()  == 0 )
            cc.reset(new Constant(name, &cc));
        else
            delete name;
        
        cnst_const_sptr ccr =cc;
        return ccr;
    }
    cnst_fn_sptr ExpressionPool::getFn(PoolKey& key){
        fnlock.lock_shared();
        auto it = fnPool.find(key.name);
        if( it != fnPool.end() ){
            //function name exists!
            key.name = nullptr; 
            FnPool& fp = it->second;
            auto itp = fp.find(key);
            if( itp != fp.end()){
                //function exists!
                fn_sptr& fn = itp->second;
                //check if deleted
                if(fn.use_count() == 0){ //Has been deleted
                    fnlock.unlock_shared();
                    return reset(fn, it->first, key, fnlock);
                }
                else{
                    fn_sptr fnr = fn;
                    fnlock.unlock_shared();
                    delete key.body;
                    key.body = nullptr;
                    return fnr;
                }
            }
            else{
                fnlock.unlock_shared();
                //Doesn't exist
                //share name i.e, it->first
                return add<Function,FnPool>(fp, it->first, key, fnlock);
            }
        }
        fnlock.unlock_shared();
        //Both the name and the do not exist
        return add<Function,NameFnMap>(fnPool,key, fnlock);
    }
    cnst_lit_sptr ExpressionPool::getLiteral(PoolKey& key){
        litlock.lock_shared();
        auto it = litPool.find(key.name);
        if( it != litPool.end()){
            key.name = nullptr; 
            //Literal name exists
            auto& lp = it->second;
            auto itp = lp.find(key);
            if( itp != lp.end() ){
                //Literal exists
                lit_sptr& l = itp->second;
                if(l.use_count() == 0 ){//could've been deleted
                    litlock.unlock_shared();
                    return reset(l, it->first, key, litlock);
                }
                else{
                    cnst_lit_sptr lr =l;
                    litlock.unlock_shared();
                    key.body = nullptr;
                    delete key.body;
                    return lr;
                }
            }
            else{
                //Haven't seen literal before add it.
                litlock.unlock_shared();
                return add<Literal, LitPool>( lp, it->first, key,litlock);
            }
        }
         //Both name and body do not exist 
        litlock.unlock_shared();
        // lit_sptr& l = litPool[name][key];
        return add<Literal,NameLitMap>(litPool,key, litlock);
    }

    /**
    * Poll the deletion pool and reset queued shared pointers
    * if the only reference that exists is within the pool.
    */
    void ExpressionPool::remove(DeletionQueue& queue, boost::shared_mutex& lock){
        while (!pollDone)
        {
            sleep(cfg.deletionPeriod);
            {
                // std::lock_guard<boost::shared_mutex> lk(lock);//Exclusively lock the pool
                // /**
                //  * if use_count() == 1 then the only copy that exists is within
                //  * the expression pool. So delete it. 
                //  */
                // auto _reset = [&](const structured_term* st){
                //     /* use_count could be > 1 b/c st could be reused between the time
                //         * its queued for deletion and its actuall deletion. 
                //         */
                //     if( st->_this->use_count() > 1 ) return; 
                //     st->_this->reset();
                // };
                // queue.apply<decltype(_reset)>(_reset);
            }
        }
    }
} // namespace ares
