#ifndef TEST_MEMPOOL_HH
#define TEST_MEMPOOL_HH
#ifndef EXPRESSIONPOOL_HH
#define EXPRESSIONPOOL_HH
#ifndef ARES_HH
#define ARES_HH

#include "utils/memory/memoryPool.hh"
#include "utils/memory/body.hh"
#include "utils/gdl/gdl.hh"
#include <assert.h>
#include <random>
#include "common.hh"
#include <algorithm>
namespace ares
{
    /**
     * Mock The ExpressionPool Class for unit testing the Memory pool.
     */
    class ExpressionPool
    {
    private:
        /* data */
    public:
        ExpressionPool(/* args */) {}
        
        cnst_var_sptr getVar(const char* name ){
            return cnst_var_sptr(new Variable(name));
        }
        cnst_const_sptr getConst(const char* name ){
            return cnst_const_sptr(new Constant(name));
        }
        cnst_fn_sptr getFn(PoolKey& key ){
            return cnst_fn_sptr(new Function(key.name, key.body));
        }
        cnst_lit_sptr getLiteral(PoolKey& key,bool save=false ){
            auto l =cnst_lit_sptr(new Literal(key.name, key.p, key.body));
            if (save)
                pool.push_back(l);
            return l;
        }
        bool remove(structured_term* st){
            auto it = std::find_if(pool.begin(), pool.end(),[&](cnst_lit_sptr& _s){return _s.get() == st;});
            if( it != pool.end()){
                pool.erase(it);
            }
            return true;
        }
        std::vector<cnst_lit_sptr> pool;
        ~ExpressionPool() {}
    };

    class Ares
    {
    private:
        /* data */
    public:
        Ares(/* args */) {}
        ~Ares() {}
        static ExpressionPool* exprpool;
        static MemoryPool* mempool;
    };
    ExpressionPool* Ares::exprpool = nullptr;
    MemoryPool* Ares::mempool = nullptr;
        
    //Initialize static members of term
    CharpHasher Term::nameHasher;
    std::shared_ptr<const Term>     Term::null_term_sptr(nullptr);
    template<class T>
    MemoryPool* _Body<T>::mempool =nullptr;
} // namespace ares

using namespace ares;

struct rand_pool_info
{
    /**
     * number of terms pre allocated for
     */
    uint term_num;                      
    /**
     * number of clauses pre allocated for
     */
    uint clause_num;                    
    /**
     * distinct term arities
     */
    uint dis_arites;      
    uint body_num;              
    /**
     * the random arrities of terms     
     */ 
    std::vector<std::pair<arity_t, uint>> arities;

    MemoryPool* mempool ;
};



rand_pool_info createPool(){
    srand(time(NULL));
    rand_pool_info rpi;
    rpi.term_num = (rand() % 401 ) + 100;
    rpi.clause_num = (rand() % 401 ) + 100;
    rpi.dis_arites = (rand() % 20 ) + 1;
    rpi.body_num = rpi.term_num + rpi.clause_num;
    rpi.arities.resize(rpi.dis_arites);
    for (size_t j = 0; j < rpi.dis_arites; j++){
        auto i = (rand() % 20 ) + 1;
        auto per_arity = (rand() % 40) + 10;

        if( std::find_if(rpi.arities.begin(), rpi.arities.end(), [&](std::pair<arity_t, uint>& p){return p.first == i;}) == rpi.arities.end() )
            rpi.arities[j] = std::make_pair(i,per_arity );
        else  j--;
    }
    MemoryPool* mempool = new MemoryPool(rpi.term_num,rpi.clause_num,rpi.arities);
    rpi.mempool = mempool;
    Body::mempool = mempool;
    ClauseBody::mempool = mempool;
    Ares::mempool = mempool;
    ExpressionPool* exppool = new ExpressionPool();
    Ares::exprpool = exppool;
    return rpi;
}

void assert_others(rand_pool_info& rpi, std::vector<pool_type> except){
    static pool_type types[]{sterm_pool_t,clause_pool_t,body_pool_t};
    uint nums[]{rpi.term_num, rpi.clause_num, rpi.term_num};

    auto* mempool = rpi.mempool;
    for (size_t i = 0; i < 3; i++)
    {
        if(find( except.begin() , except.end() , types[i]) == except.end())
            assert_true( mempool->capacity(types[i]) == nums[i]);
    }
    
}

#include "../utils/gdl/structuredTerm.cpp"

#endif
#endif
#endif