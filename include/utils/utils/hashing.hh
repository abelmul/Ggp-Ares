#ifndef HASHING_HH
#define HASHING_HH
#include <vector>
#include <unordered_set>
#include "utils/memory/body.hh"
namespace ares
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
  
    
    struct SpVarHasher
    {
        std::size_t operator() (const cnst_var_sptr&) const;
    };

    struct SpVarEqual
    {
        bool operator()(const cnst_var_sptr&,const  cnst_var_sptr&) const;
    };

    struct PoolKey
    {
        const char* name;
        const Body* body;
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

    // template<class T>
    // class UniqueVector : public std::vector<T>{
    //     std::unordered_set<T> _elements;

    // };
} // namespace ares

#endif