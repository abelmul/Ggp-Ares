#include "ares.hh"
#include "utils/memory/memCache.hh"

#include <type_traits>

/**
 * This file defines new and delete for those managed by the
 * memory pool.
 */
namespace ares
{
    // /**
    //  * new and delete for class Function
    //  */
    void* Function::operator new(std::size_t)
    {
        return Ares::mempool->allocate(sterm_pool_t);
    }
    void Function::operator delete(void* p)
    {
        Ares::mempool->deallocate((structured_term*)p);
    }
    /**
     * new and delete for class Atom
     */
    void* Atom::operator new(std::size_t)
    {
        return Ares::mempool->allocate(sterm_pool_t);
    }
    void Atom::operator delete(void* p)
    {
        Ares::mempool->deallocate((structured_term*)p);
    }

    void* Or::operator new(std::size_t)
    {
        return Ares::mempool->allocate(sterm_pool_t);
    }
    void Or::operator delete(void* p)
    {
        Ares::mempool->deallocate((structured_term*)p);
    }
    void* Not::operator new(std::size_t)
    {
        return Ares::mempool->allocate(sterm_pool_t);
    }
    void Not::operator delete(void* p)
    {
        Ares::mempool->deallocate((structured_term*)p);
    }
    /**
     * new and delete for class Clause.
     */
    void* Clause::operator new(std::size_t)
    {
        return Ares::mempool->allocate(clause_pool_t);
    }
    void Clause::operator delete(void* p)
    {
        Ares::mempool->deallocate((Clause*)p);
    }
    /**
     * Instantiation of Functions and Literals.
     */
    Body* instantiate(const structured_term& expr, const Substitution& sub,
                      VarSet& vSet);

    const Term* Function::operator()(const Substitution& sub,
                                     VarSet& vSet) const
    {
        PoolKey key{name, instantiate(*this, sub, vSet)};
        if (!key.body)
            return nullptr;
        return Ares::memCache->getFn(key);
    }

    const Term* Atom::operator()(const Substitution& sub, VarSet& vSet) const
    {
        PoolKey key{name, instantiate(*this, sub, vSet)};
        if (!key.body)
            return nullptr;
        return Ares::memCache->getAtom(key);
    }
    const Term* Or::operator()(const Substitution& sub, VarSet& vSet) const
    {
        PoolKey key{name, instantiate(*this, sub, vSet)};
        if (!key.body)
            return nullptr;
        return Ares::memCache->getOr(key);
    }
    const Term* Not::operator()(const Substitution& sub, VarSet& vSet) const
    {
        PoolKey key{name, instantiate(*this, sub, vSet)};
        if (!key.body)
            return nullptr;
        return Ares::memCache->getNot(key);
    }
    Body* instantiate(const structured_term& expr, const Substitution& sub,
                      VarSet& vSet)
    {
        uint arity = expr.arity();
        Body* body = new Body(arity);
        const Body& bodyExp = expr.getBody();
        for (uint i = 0; i < arity; i++) {
            const Term* arg = bodyExp[i];
            const Term* argInst = (*arg)(sub, vSet);
            if (not argInst) {
                delete body;
                return nullptr;  // Must have detected a loop
            }
            (*body)[i] = argInst;
        }
        return body;
    }
}  // namespace ares
