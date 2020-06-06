#include "reasoner/substitution.hh"
#include "utils/gdl/variable.hh"
#include "utils/gdl/term.hh"
#include <string.h>

namespace Ares
{
    std::size_t VarHasher::operator() (const Variable* x) const{
        const char * name = x->getName();
        unsigned long hash = 5381;
        int c;

        while ( (c = *name++))
            hash = ((hash << 5) + hash) + c;

        return hash;
    }
    
    bool VarEqual::operator()(const Variable *v1, const Variable *v2) const{
        return v1 == v2;
    }
    
    Substitution Substitution::emptySub;

    bool  Substitution::bind(Variable* x, Term* t){
        //This variable has already been bound!
        if(mappping.find(x) != mappping.end()) return false;
        mappping[x] = t;
        return true;
    }
    Term* Substitution::get(Variable* x){
        return mappping[x];
    }
    //Overload the indexing operator, to get the underlying mapping        
    std::string Substitution::operator[](Variable* x){
        if( not isBound(x)) return nullptr;
        
        VarSet vSet;
        vSet.insert(x);
        Term* t = get(x);
        return (*t)(*this,vSet);
    }

    bool Substitution::isBound(Variable* x){
        return mappping.find(x) != mappping.end();
    }
    
    Substitution* Substitution::operator +(Substitution& sub){
        Substitution* sNew = new Substitution();
        sNew->mappping.insert(mappping.begin(),mappping.end());
        sNew->mappping.insert(sub.mappping.begin(),sub.mappping.end());
        return sNew;
    }   

    Substitution::~Substitution()
    {
    }

} // namespace Ares
