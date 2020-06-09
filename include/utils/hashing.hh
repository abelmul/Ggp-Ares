#ifndef HASHING_HH
#define HASHING_HH
#include <vector>

namespace Ares
{
    class Variable;
    class Term;
    
    struct CharpHasher
    {
        std::size_t operator() (const char* name) const;
    };
    struct StrEq
    {
        bool operator()(const char* s1, const char* s2) const;
    };
    //Used in Substitutions to index using a variable
    struct VarHasher
    {
        std::size_t operator() (const Variable* x) const;
    };
    struct VarEqual
    {
        bool operator()(const Variable *v1, const Variable *v2) const;
    };

    struct PoolKey
    {
        const char* name;
        const std::vector<Term*>* body;
        bool p = true;
    };

    struct PoolKeyHasher
    {
        std::size_t operator() (const PoolKey& k) const;
    };
    
    struct PoolKeyEqual
    {
        bool operator()(const PoolKey& v1, const PoolKey& v2) const;
    };
} // namespace Ares

#endif