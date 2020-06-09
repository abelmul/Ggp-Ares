#include "utils/hashing.hh"
#include "reasoner/substitution.hh"
#include "utils/gdl/variable.hh"
#include "utils/gdl/term.hh"
#include <string.h>

namespace Ares
{   

    std::size_t CharpHasher::operator()(const char* name) const{
        unsigned long hash = 5381;
        int c;

        while ( (c = *name++))
            hash = ((hash << 5) + hash) + c;

        return hash;
    }
    
    std::size_t VarHasher::operator() (const Variable* x) const{
        return x->hash();
    }
    
    bool VarEqual::operator()(const Variable *v1, const Variable *v2) const{
        return v1 == v2;
    }
    bool StrEq::operator()(const char* s1, const char* s2) const{
        return strcasecmp(s1,s2) == 0;
    }
    
    std::size_t PoolKeyHasher::operator() (const PoolKey& k) const{
        std::size_t nHash = std::hash<bool>()(k.p);
        for (Term* t: *k.body)
            hash_combine(nHash, t);
        return nHash;
    }
    bool PoolKeyEqual::operator()(const PoolKey& k1, const PoolKey& k2) const{
        if( (k1.body->size() != k2.body->size()) ) return false;

        for (size_t i = 0; i < k1.body->size(); i++)
            if( k1.body->at(i) != k2.body->at(i))
                return false;

        return true;
    }
} // namespace Ares
