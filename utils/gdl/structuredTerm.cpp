#include "utils/memory/memCache.hh"
#include <type_traits>
#include "ares.hh"

/**
 * This file defines new and delete for those managed by the 
 * memory pool. 
 */
namespace ares
{   
    // /**
    //  * new and delete for class Function
    //  */
    void* Function::operator new(std::size_t){
        return Ares::mempool->allocate(sterm_pool_t);
    }
    void Function::operator delete(void *p){
        Ares::mempool->deallocate((structured_term*) p);
    }
    /**
     * new and delete for class Literal
     */
    void* Literal::operator new(std::size_t){
        return Ares::mempool->allocate(sterm_pool_t);
    }
    void Literal::operator delete(void *p){
        Ares::mempool->deallocate((structured_term*) p);
    }
    /**
     * new and delete for class Clause.
     */
    void* Clause::operator new(std::size_t){
        return Ares::mempool->allocate(clause_pool_t);
    }
    void Clause::operator delete(void *p){
        Ares::mempool->deallocate((Clause*) p);
    }
    /**
     * Instantiation of Functions and Literals.
     */
    template<class T>
    Body* instantiate(const T& expr,const Substitution &sub,VarSet& vSet);

    const cnst_term_sptr Function::operator()(const Substitution &sub,VarSet& vSet) const {
        Body* body = instantiate<Function>(*this, sub, vSet);
        if( !body ) return null_term_sptr;
        PoolKey key{name, body,true};
        return Ares::memCache->getFn(key);
    }

   const cnst_term_sptr Literal::operator()(const Substitution &sub,VarSet& vSet) const {

        Body* body = instantiate<Literal>(*this, sub, vSet);
        if( !body ) return null_term_sptr;
        PoolKey key{name, body,this->positive};
        return Ares::memCache->getLiteral(key);
    }

    template<class T>
    Body* instantiate(const T& expr, const Substitution &sub,VarSet& vSet){
        uint arity = expr.getArity();
        Body* body = new Body(arity);
        const Body& bodyExp = expr.getBody();
        for (uint i =0; i < arity; i++)
        {
            const cnst_term_sptr& arg = bodyExp[i];
            const cnst_term_sptr& argInst = (*arg)(sub, vSet);
            if( not argInst ){
                delete body;
                return nullptr;     //Must have detected a loop
            }
            (*body)[i] = argInst;
        }
        /**
         * TODO:
         * Creation and deletion of body could be avoided if Expression pool is 
         * Responsible for the lifecycle of Function Bodies.
         */
        return body;
    }
} // namespace ares
