#ifndef MEMCACHE_HH
#define MEMCACHE_HH

#include "utils/gdl/gdl.hh"
#include "utils/utils/hashing.hh"
#include "utils/memory/memoryPool.hh"
#include "utils/threading/threading.hh"
#include "utils/memory/queue.hh"
#include "utils/memory/namer.hh"

namespace ares
{

    /**
     * Responsible for the creation and deletion of expressions,
     * expressions are, names(char*) of predicates and terms, predicates and terms.
     */
    class MemCache
    {
        typedef std::shared_ptr<Variable> sVariable;
        typedef std::shared_ptr<Constant> sConstant;
        typedef std::shared_ptr<Function> sFunction;
        typedef std::shared_ptr<Atom> sLiteral;
        typedef std::shared_ptr<Or> sOr;
        typedef std::shared_ptr<Not> sNot;

        typedef tbb::concurrent_hash_map<ushort, sVariable> VarPool;
        typedef tbb::concurrent_hash_map<ushort, sConstant> ConstPool;

        typedef tbb::concurrent_hash_map<PoolKey, sFunction,PoolKeyHasher> FnPool;
        typedef tbb::concurrent_hash_map<PoolKey, sLiteral,PoolKeyHasher> AtomPool;
        typedef tbb::concurrent_hash_map<PoolKey, sOr,PoolKeyHasher> OrPool;
        typedef tbb::concurrent_hash_map<PoolKey, sNot,PoolKeyHasher> NotPool;

        typedef tbb::concurrent_hash_map<ushort, FnPool> NameFnMap;
        typedef tbb::concurrent_hash_map<ushort, AtomPool> NameAtomMap;

        typedef std::chrono::seconds seconds;

        friend class MemoryPool;
    //ctor
    private:
        /**
         * Only a single Memory cache should exist through out the life time of the application
         * and its created by the singleton MemoryPool.
         */
        MemCache(){}

        MemCache(const MemCache&)=delete;
        MemCache& operator=(const MemCache&)=delete;
        MemCache(const MemCache&&)=delete;
        MemCache& operator=(const MemCache&&)=delete;
    /**
     * Methods
     */
    public:

        /**
         * The get* methods make sure only one instance of an object exists.
         * sets @param exists to true if the expression exists. if exists
         * They are all thread safe. 
         */
        const Variable* getVar(ushort var);
        const Constant* getConst(ushort c);
        /**
         * if the function is in the pool, then the existing function is returned. 
         * else a new function with name key.name and body key.body is created. 
         * key.body is now owned by MemCache, and should neither be deleted nor
         * assigned to other objects at any point. This is to avoid copying the body while creation.
         */
        const Function* getFn(PoolKey& key);
        /**
         * if the literal is in the pool, then the existing literal is returned. 
         * else a new literal with name key.name and body key.body is created. 
         * key.body is now owned by MemCache, and should neither be deleted nor
         * assigned to other objects at any point. This is to avoid copying the body while creation.
         */
        const Atom* getAtom(PoolKey& key);
        const Or* getOr(PoolKey& key);
        const Not* getNot(PoolKey& key);
        void clear();
        template<class T> void deleter(T* t){ delete t;}
        ~MemCache();
        
    /**
     * Data
     */
    private:

        VarPool varPool;
        ConstPool constPool;
        NameFnMap nameFnPool;
        NameAtomMap nameAtomPool;
        OrPool orPool;
        NotPool notPool;

    };
} // namespace ares

#endif