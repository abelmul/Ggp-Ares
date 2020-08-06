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
        ushort name;
        const Body* body=nullptr;
        bool p = true;
        structured_term* _this;
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

        public:
            UniqueVector() = default;

            UniqueVector(std::size_t s){
                elements.reserve(s);
                lookup.reserve(s);
            }
            void push_back(T el){
                //check if el exists
                auto it = lookup.find(el);
                if( it == lookup.end()){
                    // Doesn't exist, remember it
                    lookup.insert(el);
                    elements.push_back(el);
                }
            }

            inline bool exists(const T& el){
                if( lookup.find(el) == lookup.end()) return false;

                return true;
            }
            inline void clear(){
                elements.resize(0);
                lookup.clear();
            }
            inline std::size_t size()const { return elements.size();}
            iterator begin()const { return elements.cbegin();}
            iterator end()const { return elements.cend();}

        private:
            std::vector<T> elements;
            std::unordered_set<T,Hash,Eq> lookup;
    };
} // namespace ares

#endif