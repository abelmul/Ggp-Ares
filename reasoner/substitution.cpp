#include "reasoner/substitution.hh"
#include "utils/gdl/variable.hh"
#include "utils/gdl/term.hh"
#include <string.h>

namespace Ares
{
    Substitution Substitution::emptySub;

    bool  Substitution::bind(const Variable* x,const Term* t){
        //This variable has already been bound!
        if(mappping.find(x) != mappping.end()) return false;
        mappping[x] = t;
        return true;
    }
    const Term* Substitution::get(const Variable* x) const {
        return mappping.at(x);
    }
    //Overload the indexing operator, to get the underlying mapping        
    const Term* Substitution::operator[] (const Variable* x) const {
        if( not isBound(x)) return nullptr;
        
        VarSet vSet;
        vSet.insert(x);
        const Term* t = get(x);
        return (*t)(*this,vSet);    //Walk through the implicit `chain`
    }

    bool Substitution::isBound(const Variable* x) const {
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
