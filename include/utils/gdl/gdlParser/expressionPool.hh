#ifndef EXPRESSIONPOOL_HH
#define EXPRESSIONPOOL_HH

#include "utils/gdl/term.hh"
#include "utils/gdl/variable.hh"
#include "utils/gdl/constant.hh"
#include "utils/gdl/function.hh"
#include "utils/gdl/literal.hh"
#include "utils/hashing.hh"

#include <boost/thread/pthread/shared_mutex.hpp>

namespace Ares
{
    /**
     * Responsible for the creation and deletion of expressions,
     * expressions are, names(char*) of predicates and terms,predicates and terms.
     */
    class ExpressionPool
    {

    public:
        typedef std::unordered_map<PoolKey*, Function*,PoolKeyHasher,PoolKeyEqual> FnPool;
        typedef std::unordered_map<PoolKey*, Literal*,PoolKeyHasher,PoolKeyEqual> LitPool;

        ExpressionPool(/* args */){}
        /**
         * The get* methods make sure only one instance of an object exists.
         * sets @param exists to true if the expression exists. if exists
         * They are all thread safe. 
         */
        Variable* getVar(const char* var);
        Constant* getConst(const char* c);
        /**
         * if @param exists is true, then the existing function is returned. 
         * else a new function with name key.name and body key.body is created. 
         * key.body is now owned by ExpressionPool, and should neither be deleted nor
         * assigned to other objects at any point. This is to avoid copying the body while creation.
         */
        Function* getFn(PoolKey& key, bool& exists);
        /**
         * if @param exists is true, then the existing literal is returned. 
         * else a new literal with name key.name and body key.body is created. 
         * key.body is now owned by ExpressionPool, and should neither be deleted nor
         * assigned to other objects at any point. This is to avoid copying the body while creation.
         */
        Literal* getLiteral(PoolKey& key, bool& exists);

        ~ExpressionPool(){}

    private:
        std::unordered_map<const char*, Variable*,CharpHasher,StrEq> varPool;
        std::unordered_map<const char*, Constant*,CharpHasher,StrEq> constPool;
        std::unordered_map<const char*, FnPool, CharpHasher,StrEq> fnPool;
        std::unordered_map<const char*, LitPool,CharpHasher,StrEq> litPool;

        /** 
         * The below mutexes provide multiple(shared) readers/single(exclusive) writer access to 
         * their respective pools.
         */        
        boost::shared_mutex varlock;
        boost::shared_mutex constlock;
        boost::shared_mutex fnlock;
        boost::shared_mutex litlock;
    };
} // namespace Ares

#endif