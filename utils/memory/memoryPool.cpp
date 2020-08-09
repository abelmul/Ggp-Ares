#include "utils/memory/memoryPool.hh"
#include "utils/memory/memCache.hh"
#include "utils/gdl/gdl.hh"
#include "ares.hh"

namespace ares
{

    const lit_container* MemoryPool::EMPTY_CONTAINER  = new lit_container();
    MemCache* MemoryPool::memCache = nullptr;

    MemoryPool::MemoryPool(std::size_t st_terms,std::size_t clause_s,std::vector<std::pair<arity_t,uint>> arites){
        //Just for ease of access.
        memCache  = new MemCache();
        pool_element_size[sterm_pool_t]  = sizeof(structured_term);
        pool_element_size[clause_pool_t] = sizeof(Clause);
        pool_element_size[body_pool_t]   = sizeof(Body);

        POOLS = std::vector<std::vector<void*>*>{&st_term_pool, &clause_pool,&body_pool};
        //initialize the pools.
        init_pools(st_terms,clause_s,arites);
    }
    MemoryPool::~MemoryPool(){
        //Return any thing cached back to the free pool
        delete memCache;

        //Delete everything allocated so far
        for (std::vector<void*>* v: POOLS)
            for (void* vp : *v)
                free(vp);
                    
        for (auto&& it: container_pool)
            for (void* vp : it.second.second)
                delete ((term_container*)vp);
        // delete EMPTY_CONTAINER;
    }
    void MemoryPool::init_pools(
        std::size_t st_terms,
        std::size_t clause_s,
        std::vector<std::pair<arity_t,uint>> arities
        ){
            /* Pre allocate term containers i.e vector<shared_ptr<Term>>, of given arities*/
            init_pool(arities);

            /*Pre allocate terms and Bodies of terms.*/
            init_pool<structured_term>(st_term_pool, st_terms);
            init_pool<Body>(body_pool, st_terms + clause_s);
            init_pool<Clause>(clause_pool, clause_s);
            
    }
    /**
     * @param type could be sterm_pool_t,clause_pool_t  ,body_pool_t
     * @returns a structured_term* from the free pool. if type == sterm_pool_t
     * @returns a Clause* from the free Clause pool. if type == clause_pool_t
     * @returns a _Body<T>* from the free Clause pool. if type == body_pool_t
     */
    void* MemoryPool::allocate(pool_type type){
        std::lock_guard<SpinLock> lk(slock[type]);
        //get the corresponding pool 
        std::vector<void*>& pool =  *POOLS.at(type);

        if( pool.size() == 0 )  grow(pool,type);
        return allocate(pool);
    }    
    /**
     * @returns a container<cnst_term_sptr>* or container<cnst_lit_sptr>* from the free pool
     */
    void* MemoryPool::allocate(arity_t arity){
        std::lock_guard<SpinLock> lk(slock[container_pool_t]);    
        std::vector<void*>& pool = ar_pool(arity);
        if( pool.size()==0) grow(pool, ar_gfactor(arity),arity);
        return allocate(pool);
    }
    /**
     * Return @param st a structured_term back to the free pool to be reused.
     */
    void MemoryPool::deallocate(structured_term* st){
        std::lock_guard<SpinLock> lk(slock[sterm_pool_t]);
        st_term_pool.push_back(st);
    }
    /**
     * Return @param c,a Clause ptr, back to the free pool to be reused.
     */
    void MemoryPool::deallocate(Clause* c){
        std::lock_guard<SpinLock> lk(slock[clause_pool_t]);
        clause_pool.push_back(c);
    }
    /**
     * Return @param vec back to the free pool to be reused.
     */
    void MemoryPool::deallocate(cnst_term_container* vec){
        std::lock_guard<SpinLock> lk(slock[container_pool_t]);
        ar_pool(vec->size()).push_back(vec);
    }

    void MemoryPool::deallocate(cnst_lit_container* vec){
        deallocate((cnst_term_container*) vec);
    }
} // namespace ares


