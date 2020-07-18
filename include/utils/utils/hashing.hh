#ifndef HASHING_HH
#define HASHING_HH
#include <vector>
#include <unordered_set>
#include "utils/memory/body.hh"
#include <algorithm>

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

    struct ClauseHasher
    {
        std::size_t operator() (const Clause*) const;
        bool operator()(const Clause* c,const Clause* c2) const;
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

    template<class T,class Hash = std::hash<T>, class Eq=std::equal_to<T>>
    class UniqueVector{
        typedef typename std::vector<T>::const_iterator iterator;
        typedef std::size_t hash_t;
        typedef std::vector<T> bucket_t;
        public:
            UniqueVector() = default;

            UniqueVector(std::size_t s){
                elements.reserve(s);
                lookup.reserve(s);
            }
            void push_back(T el){
                //check if el exists
                hash_t h = hash(el);
                auto it = lookup.find(h);
                if( it == lookup.end()){
                    // Doesn't exist
                    //remember it
                    lookup[h].push_back(el);
                    elements.push_back(el);
                }
                else{
                    //exists
                    bucket_t& bk = it->second;

                    auto itf = std::find_if(bk.begin(), bk.end(),[&](T& e){return eq(e, el);});
                    if( itf == bk.end()) bk.push_back(el);
                }
            }
            void clear(){
                elements.resize(0);
                lookup.clear();
            }

            iterator begin()const { return elements.cbegin();}
            iterator end()const { return elements.cend();}

        private:
            std::vector<T> elements;
            std::unordered_map<hash_t,bucket_t> lookup;
            Hash hash;
            Eq eq;
    };
} // namespace ares

#endif