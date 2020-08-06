#include "utils/memory/expressionPool.hh"
#include <chrono> 

namespace ares
{
    cnst_var_sptr ExpressionPool::getVar(ushort n){
        timer t(this);
        varlock.lock_shared();
        if( varPool.find(n) != varPool.end()){
            cnst_var_sptr v = varPool[n];
            varlock.unlock_shared();
            return v;
        }
        varlock.unlock_shared();
        //Exclusive ownership for writing
        std::lock_guard<boost::shared_mutex> lk(varlock);
        cnst_var_sptr& v = varPool[n];
        if( v.use_count() == 0)
            v.reset( new Variable(n,&v));

        cnst_var_sptr vr = v;
        return vr;
    }
    cnst_const_sptr ExpressionPool::getConst(ushort c){
        timer t(this);
        constlock.lock_shared();
        if( constPool.find(c) != constPool.end() ){
            cnst_const_sptr cc = constPool[c];
            constlock.unlock_shared();
            return cc;
        }
        constlock.unlock_shared();
        //upgrade lock to exclusive
        std::lock_guard<boost::shared_mutex> lk(constlock);
        cnst_const_sptr& cc = constPool[c];
        if( cc.use_count()  == 0 )
            cc.reset(new Constant(c, &cc));
      
        
        cnst_const_sptr ccr =cc;
        return ccr;
    }
    cnst_fn_sptr ExpressionPool::getFn(PoolKey& key){
        timer t(this);
        fnlock.lock_shared();
        auto it = fnPool.find(key.name);
        if( it != fnPool.end() ){
            //function name exists!
            FnPool& fp = it->second;
            auto itp = fp.find(key);
            if( itp != fp.end()){
                //function exists!
                //check if deleted/hasn't expired
                if(auto fn = itp->second.lock()){ //Hasn't been deleted
                    fnlock.unlock_shared();
                    delete key.body;
                    key.body = nullptr;
                    return fn;
                }
                else{//Has been deleted
                    fnlock.unlock_shared();
                    return reset<Function, FnPool>(fp, it->first, key, fnlock);
                }
            }
            else{
                fnlock.unlock_shared();
                //Doesn't exist
                //share name i.e, it->first
                return reset<Function,FnPool>(fp, it->first, key, fnlock);
            }
        }
        fnlock.unlock_shared();
        //Both the name and the do not exist
        return add<Function,NameFnMap>(fnPool,key, fnlock);
    }
    cnst_lit_sptr ExpressionPool::getLiteral(PoolKey& key){
        timer t(this);
        litlock.lock_shared();
        auto it = litPool.find(key.name);
        if( it != litPool.end()){
            //Literal name exists
            auto& lp = it->second;
            auto itp = lp.find(key);
            if( itp != lp.end() ){
                //Literal exists
                //could've been deleted
                if(auto l = itp->second.lock()){
                    litlock.unlock_shared();
                    delete key.body;
                    key.body = nullptr;
                    return l;
                }
                else{
                    litlock.unlock_shared();
                    return reset<Literal>(lp, it->first, key, litlock);
                }
            }
            else{
                //Haven't seen literal before add it.
                litlock.unlock_shared();
                return reset<Literal, LitPool>( lp, it->first, key,litlock);
            }
        }
         //Both name and body do not exist 
        litlock.unlock_shared();
        // lit_sptr& l = litPool[name][key];
        return add<Literal,NameLitMap>(litPool,key, litlock);
    }
} // namespace ares
