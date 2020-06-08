#ifndef EXPRESSIONPOOL_HH
#define EXPRESSIONPOOL_HH

#include "utils/gdl/gdlParser/gdlParser.hh"
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
        ExpressionPool(/* args */);
        /**
         * The below get* methods make sure only one instance of an object exists.
         * sets @param exists to true if the expression exists.
         * They are all thread safe.
         */
        Variable* getVar(const char* var, bool exists);
        Constant* getConst(const char* c, bool exists);
        Function* getFn(PoolKey& fn, bool exists);
        Literal* getLiteral(PoolKey& l, bool exists);

        ~ExpressionPool();

    private:
        std::unordered_map<char*, Variable*> varPool;
        std::unordered_map<char*, Constant*> constPool;
        std::unordered_map<PoolKey, Function*,PoolKeyHasher,PoolKeyEqual> fnPool;
        std::unordered_map<PoolKey, Literal*,PoolKeyHasher,PoolKeyEqual> litPool;
        std::unordered_map<char* , char*, CharpHasher> names;

        /** 
         * The below mutexes provide multiple readers/single writer access to 
         * their respective pools.
         */        
        boost::shared_mutex varlock;
        boost::shared_mutex constlock;
        boost::shared_mutex fnlock;
        boost::shared_mutex litlock;
        boost::shared_mutex namelock;

    };
} // namespace Ares

#endif