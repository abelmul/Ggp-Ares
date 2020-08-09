#include "Test_MemPool.hh"

using namespace ares;

void Test_Init();
void Test_Body_Allocation();
void Test_ClauseBody_Allocation();
void Test_Function_Allocation();
void Test_Literal_Allocation();
void Test_Clause_Allocation();

int main(int argc, char const *argv[])
{
    Runner runner;
    runner.iter = 2000;

    TestCase initialization(Test_Init);
    TestCase functionAllocation(Test_Function_Allocation);
    TestCase literalAllocation(Test_Literal_Allocation);
    TestCase clauseAllocation(Test_Clause_Allocation);
    TestCase bodyAllocation(Test_Body_Allocation);
    TestCase clausebodyAllocation(Test_ClauseBody_Allocation);

    add_test(runner,initialization);
    add_test(runner,functionAllocation);
    add_test(runner,literalAllocation);
    add_test(runner,clauseAllocation);
    add_test(runner,bodyAllocation);
    add_test(runner,clausebodyAllocation);

    runner();
    
    return 0;
}

/**
 * Do simple tests to verify that the pool are actually created
 * according to the given parameters.
 */

void Test_Init(){
    for (size_t i = 0; i < N_ITER; i++)
    {
        //Create pool of random sizes
        auto rpi = createPool();
        auto* mempool = rpi.mempool;
        // std::cout << "created pool of : " << rpi.term_num << ""<< rpi.body_num << ", " << rpi.clause_num 
        //assert_true created size of structured_term/clause pool is as specified
        assert_true(mempool->capacity(sterm_pool_t) == rpi.term_num);
        assert_true(mempool->capacity(body_pool_t) == rpi.body_num);
        assert_true(mempool->capacity(clause_pool_t) == rpi.clause_num);

        for (auto &&i : rpi.arities)
            assert_true(mempool->capacity(container_pool_t, i.first) == i.second);
        
        // delete mempool;
    }

}

void Test_Function_Allocation(){
    auto rpi = createPool();
    auto mempool = rpi.mempool;
    //exhaust every structured term by allocating it as a function
    auto exhaust = [&](){
        std::vector<cnst_fn_sptr> fns;
        for (size_t j = 1; j <= rpi.term_num; j++)
        {
            PoolKey key{"test_fn", new Body(0)};
            cnst_fn_sptr fn = Ares::memCache->getFn(key);
            assert_true( strcmp("test_fn", fn->get_name()) == 0);
            assert_true( fn->getArity() == 0);
            assert_true( fn->get_type() == FN);
            assert_true( (bool)*fn);
            assert_true( fn->is_ground() );
            fn->hash();
            assert_true( strcmp("(test_fn)", fn->to_string().c_str()) == 0);

            assert_true( mempool->capacity( sterm_pool_t ) == (rpi.term_num-j));
            assert_true( mempool->capacity(body_pool_t) == (rpi.body_num -j));
            assert_true( mempool->capacity(clause_pool_t) == (rpi.clause_num));
            

            fns.push_back(fn);
        }
        assert_true( mempool->capacity( sterm_pool_t) == 0);
        return fns;
    };
    auto allocate_more = [&](){
        std::vector<cnst_fn_sptr> fns2;
        assert_true( mempool->capacity( sterm_pool_t) == 0);
        uint size =0, gf=1;
        for (size_t j = 1; j <= rpi.term_num; j++)
        {
            PoolKey key{"test_fn", new Body(0)};
            cnst_fn_sptr fn = Ares::memCache->getFn(key);
            assert_true( strcmp("test_fn", fn->get_name()) == 0);
            assert_true( fn->getArity() == 0);
            assert_true( fn->get_type() == FN);
            assert_true( strcmp("(test_fn)", fn->to_string().c_str()) == 0);
            if( size < j ){
                //Should grow exponentially
                size += gf;
                gf <<= 1;
            }
            fns2.push_back(fn);
        }
        assert_true( mempool->capacity( sterm_pool_t) == (size - rpi.term_num) );
        assert_true( mempool->capacity(clause_pool_t) == (rpi.clause_num));

        for (auto &&fn : fns2)
            fn.reset();
        assert( mempool->capacity ( sterm_pool_t) == size);
        return size;
    };

    auto fns = exhaust();
    //These should return all allocated functions back to the pool
    for (auto &&fn : fns)
        fn.reset();      

    assert( mempool->capacity( sterm_pool_t) == rpi.term_num);
    fns = exhaust();
    auto size = allocate_more();
    for (auto &&fn : fns)
        fn.reset();      

    assert_true( mempool->capacity( sterm_pool_t) == (size + rpi.term_num) );
    assert_true( mempool->capacity(clause_pool_t) == (rpi.clause_num));


    delete mempool;
}

void Test_Literal_Allocation(){
    for (size_t i = 0; i < N_ITER; i++)
    {
        auto rpi = createPool();
        auto mempool = rpi.mempool;
        //exhaust every structured term by allocating it as a Literal
        auto exhaust = [&](){
            std::vector<cnst_lit_sptr> lits;
            for (size_t j = 1; j <= rpi.term_num; j++)
            {
                PoolKey key{"test_lit", new Body(0)};
                cnst_lit_sptr lit = Ares::memCache->getLiteral(key);
                assert_true( strcmp("test_lit", lit->get_name()) == 0);
                assert_true( lit->getArity() == 0);
                assert_true( lit->get_type() == LIT);
                assert_true( (bool)*lit);
                assert_true( lit->is_ground() );
                lit->hash();
                assert_true( strcmp("(test_lit)", lit->to_string().c_str()) == 0);
                
                assert_true( mempool->capacity( sterm_pool_t ) == (rpi.term_num-j));
                assert_true( mempool->capacity(body_pool_t) == (rpi.body_num -j));
                assert_true( mempool->capacity(clause_pool_t) == (rpi.clause_num));
                
                
                lits.push_back(lit);
            }

            assert_true( mempool->capacity( sterm_pool_t) == 0);
            return lits;
        };
        auto allocate_more = [&](){
            std::vector<cnst_lit_sptr> lits2;

            assert_true( mempool->capacity( sterm_pool_t) == 0);
            uint size =0, gf=1;
            for (size_t j = 1; j <= rpi.term_num; j++)
            {
                PoolKey key{"test_lit", new Body(0)};
                cnst_lit_sptr lit = Ares::memCache->getLiteral(key);                
                assert_true( strcmp("test_lit", lit->get_name()) == 0);
                assert_true( lit->getArity() == 0);
                assert_true( lit->get_type() == LIT);
                assert_true( strcmp("(test_lit)", lit->to_string().c_str()) == 0);
                if( size < j ){
                    //Should grow exponentially
                    size += gf;
                    gf <<= 1;
                }
                lits2.push_back(lit);
            }
            assert_true( mempool->capacity( sterm_pool_t) == (size - rpi.term_num) );
            assert_true( mempool->capacity(clause_pool_t) == (rpi.clause_num));

            for (auto &&lit : lits2)
                lit.reset();
            
            assert( mempool->capacity ( sterm_pool_t) == size);
            return size;
        };
        auto lits = exhaust();
        for (auto &&lit : lits)
                lit.reset();      

        assert( mempool->capacity( sterm_pool_t) == rpi.term_num);
        lits = exhaust();
        auto size = allocate_more();
        //These should return all allocated Literals back to the pool
        for (auto &&lit : lits)
                lit.reset();
        assert_true( mempool->capacity( sterm_pool_t) == (size + rpi.term_num) );
        assert_true( mempool->capacity(clause_pool_t) == (rpi.clause_num));

        delete(mempool);
    }
}

void Test_Clause_Allocation(){
    for (size_t i = 0; i < N_ITER; i++)
    {
        auto rpi = createPool();
        auto mempool = rpi.mempool;
        
        auto exhaust = [&](){
            std::vector<Clause*> clauses;
            //exhaust every structured term by allocating it as a Clause
            for (size_t j = 1; j <= rpi.clause_num; j++)
            {
                Clause* c = new Clause(nullptr, new ClauseBody(0));
                
                assert_true( mempool->capacity(clause_pool_t) == (rpi.clause_num -j));
                assert_true ( mempool->capacity( body_pool_t ) == (rpi.body_num-j));
                assert_true( mempool->capacity(sterm_pool_t) == (rpi.term_num));

                assert( std::find_if(clauses.begin(),clauses.end(),[&](Clause* _c){return _c==c;})== clauses.end() );
                clauses.push_back( c );
            }
            assert_true( mempool->capacity(clause_pool_t) == 0);

            return clauses;
        };

        auto allocate_more = [&](std::vector<Clause*>& prev){
            assert_true( mempool->capacity(clause_pool_t) == 0);
            std::vector<Clause*> clauses;
            //exhaust every structured term by allocating it as a Clause
            uint size=0, gf=1;
            for (size_t j = 1; j <= rpi.clause_num; j++)
            {
                Clause* c = new Clause(nullptr, new ClauseBody(0));
                assert( std::find_if(clauses.begin(),clauses.end(),[&](Clause* _c){return _c==c;})== clauses.end() );
                assert( std::find_if(prev.begin(),prev.end(),[&](Clause* _c){return _c==c;})== prev.end() );

                clauses.push_back( c );

                if( size < j){
                    size += gf;
                    gf <<= 1;
                }
                assert_true( mempool->capacity(clause_pool_t) == (size -j));
                assert_true( mempool->capacity(sterm_pool_t) == (rpi.term_num));
            }
            for (auto &&c : clauses)
                delete c;
            
            return size;
        };
        auto clauses = exhaust();
        for (auto &&c : clauses)
            delete c;
        assert_true( mempool->capacity( clause_pool_t) == rpi.clause_num);

        clauses = exhaust();
        auto size = allocate_more(clauses);
        for (auto &&c : clauses)
            delete c;
        
        assert_true( mempool->capacity( clause_pool_t) == (rpi.clause_num + size ));
        assert_true( mempool->capacity(sterm_pool_t) == (rpi.term_num));
        delete mempool;
    }
}

/* Sub Test cases. */
struct exhaust_body_pool {void operator()(MemoryPool* mempool, rand_pool_info& rpi,std::vector<Body*>& bodies);};
struct allocate_more {void operator()(MemoryPool* mempool);};

void Test_Body_Allocation(){
    exhaust_body_pool exhaust;
    allocate_more allocate;

    for (size_t i = 0; i < N_ITER; i++)
    {
        auto rpi = createPool();
        auto* mempool = rpi.mempool;
        std::vector<Body*> bodies;
        //exhaust every body of arity that was pre allocated
        exhaust(mempool,rpi,bodies); 

        //Return back to pool
        for (auto &&b : bodies)
            delete b;
        //assert_true that each body/term container is returned back to its correct pool.
        for (auto &&arit : rpi.arities)
            assert_true(mempool->capacity(container_pool_t, arit.first) == arit.second);
        
        exhaust(mempool,rpi,bodies);
        //assert_true 1. body of all arities should be empty, 2. Should be capable of growing exponentialy
        allocate(mempool);
        delete mempool;
    }
}

void exhaust_body_pool::operator()(MemoryPool* mempool, rand_pool_info& rpi,std::vector<Body*>& bodies){
    for (auto &&arity : rpi.arities)
    {
        for (size_t j = 1; j <=arity.second; j++)
        {
            Body* body = new Body(arity.first);     
            assert_true( body->size() == arity.first );
            assert_true( mempool->capacity(container_pool_t, arity.first) == (arity.second - j) );
            for (size_t k = 0; k < arity.first; k++){
                assert_true( (*body)[k].use_count() == 0);
                (*body)[k] = Ares::memCache->getVar("?place_holder");
            }
            uint amount = (rand() % arity.first);
            for (size_t k = 0; k <= amount; k++)
                body->pop_front();
            
            bodies.push_back(body);
        }   
    }
    for (auto &&ar : rpi.arities)
        assert_true( mempool->capacity(container_pool_t,ar.first) == 0);
};
void allocate_more::operator()(MemoryPool* mempool){
    std::vector<arity_t> arities;
    std::vector<Body*> bodies;
    uint amounts = (rand() % 30 )+ 1;
    for (size_t i = 0; i < amounts; i++)
        arities.push_back((rand() %30) + 1);
    
    for (auto &&ar : arities)
        assert_true( mempool->capacity(container_pool_t,ar) == 0);

    uint size_body=0;
    for (auto &&ar : arities)
    {
        uint size=0, gf=1;
        for (size_t i = 1; i <= N_ITER; i++)
        {
            Body* b = new Body(ar);
            assert_true(b->size() == ar);
            for (size_t j = 0; j < ar; j++)
            {
                assert_true( (*b)[j].use_count() == 0);
                (*b)[j] = Ares::memCache->getVar("?place_holder");
            }
            uint amount = (rand() % ar);
            for (size_t k = 0; k <= amount; k++)
                b->pop_front();
            
            bodies.push_back(b);

            if (size < i ){
                size += gf;
                gf <<= 1;
            }
        }
        for (auto &&b : bodies)
            delete b;
        bodies.resize(0);
        assert_true( mempool->capacity(container_pool_t, ar) == size );

        size_body += size;
    }
}    
void Test_ClauseBody_Allocation(){

    auto rpi = createPool();
    auto mempool = rpi.mempool;
    std::vector<ClauseBody*> bodies;
    std::vector<cnst_lit_sptr> sps;
    for (auto &&ar : rpi.arities)
    {
        for (size_t j = 1; j <= ar.second; j++)
        {
            ClauseBody* body = new ClauseBody(ar.first);
            assert_true(body->size() == ar.first);
            assert_true( mempool->capacity(container_pool_t, ar.first) == (ar.second - j) );
            for (size_t k = 0; k < ar.first; k++){
                assert_true( (*body)[k].use_count() == 0);
                PoolKey key {"test_lit",new Body(0), true};
                cnst_lit_sptr l = Ares::memCache->getLiteral(key,true);
                sps.push_back(l);
                (*body)[k] = l;
            }
            uint amount = (rand() % ar.first);
            for (size_t k = 0; k <= amount; k++)
                body->pop_front();
            
            
            bodies.push_back(body);
        }
    }
    for (auto &&ar : rpi.arities)
        assert_true( mempool->capacity(container_pool_t,ar.first) == 0);   

    for (auto &&b : bodies)
        delete b;

    for (auto &&ar : rpi.arities)
        assert_true( mempool->capacity(container_pool_t, ar.first) == ar.second );       
    
    // delete mempool;
}