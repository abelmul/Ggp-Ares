#ifndef EXPRESSIONPOOL_HH
#define EXPRESSIONPOOL_HH

#include "utils/gdl/gdl.hh"
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
        typedef std::unordered_map<PoolKey, Function*,PoolKeyHasher,PoolKeyEqual> FnPool;
        typedef std::unordered_map<PoolKey, Literal*,PoolKeyHasher,PoolKeyEqual> LitPool;

        ExpressionPool(){
            for (const char* c : constants)
                litPool[c];         //Creates Empty LiteralPool 
        }
        /**
         * The get* methods make sure only one instance of an object exists.
         * sets @param exists to true if the expression exists. if exists
         * They are all thread safe. 
         */
        const Variable* getVar(const char* var);
        const Constant* getConst(const char* c);
        /**
         * if @param exists is true, then the existing function is returned. 
         * else a new function with name key.name and body key.body is created. 
         * key.body is now owned by ExpressionPool, and should neither be deleted nor
         * assigned to other objects at any point. This is to avoid copying the body while creation.
         */
        const Function* getFn(PoolKey& key, bool& exists);
        /**
         * if @param exists is true, then the existing literal is returned. 
         * else a new literal with name key.name and body key.body is created. 
         * key.body is now owned by ExpressionPool, and should neither be deleted nor
         * assigned to other objects at any point. This is to avoid copying the body while creation.
         */
        const Literal* getLiteral(PoolKey& key, bool& exists);

        ~ExpressionPool(){
            //delete everything
            for (auto &&it : varPool)
                delete it.second;

            for (auto &&it : constPool)
                delete it.second;    

            for (auto &&it : fnPool){
                delete it.first;        //delete shared names
                for (auto &&fnIt : it.second)
                    delete fnIt.second;
            }

            for (auto &&it : litPool){
                if (std::find(constants.begin(), constants.end(), it.first) != constants.end())
                    delete it.first;        //delete shared, malloc'd, names

                for (auto &&litIt : it.second)
                    delete litIt.second;
            }
        }

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

        /**
         * Define the GDL specific(game independant) constants
         */

        const std::vector<const char *> constants{ ROLE,TRUE,INIT,NEXT,LEGAL,DOES,DISTINCT,GOAL,TERMINAL };
        const char* ROLE = "role";
        const char* INIT = "init";
        const char* LEGAL = "legal";
        const char* NEXT = "next";
        const char* TRUE = "true";
        const char* DOES = "does";
        const char* DISTINCT = "distinct";
        const char* GOAL = "goal";
        const char* TERMINAL = "terminal";
        const char* INPUT = "input";
        const char* BASE = "base";
    };
} // namespace Ares

#endif