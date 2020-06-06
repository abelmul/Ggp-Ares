#include "reasoner/unifier.hh"
namespace Ares
{
    bool Unifier::unifyPredicate(Literal& l1, Literal& l2, Substitution& sub){
        bool dtName = strcasecmp(l1.getName(),l2.getName()) != 0;
        bool dtSign = ( (bool) l1 ) ^ (bool) l2;
        bool dtArity = (l1.getArity() != l2.getArity() );
        if( dtName || dtSign || dtArity) 
            return false; //Can't be unified
        
        for (size_t i = 0; i < l1.getArity(); i++)
        {
            bool ok = unifyTerm(*l1.getArg(i),*l2.getArg(i),sub);
            if(not ok) return false; //Can't be unified
        }
        return true;   
    }

    bool Unifier::unifyTerm(Term& s, Term& t,Substitution& sub){
        if(s == t)
            return true;
                
        if( isVar(s) ){
            auto* s_v = (Variable *) &s;
            if( sub.isBound(s_v) ) 
                return unifyTerm(*sub.get(s_v),t,sub);
            //WARNING! No occurs check!
            sub.bind(s_v, &t);
            return true;
        }
        //S is not a variable
        if( isVar(t) ) 
            return unifyTerm(t,s,sub);
        /**
         * At this point both s and t are not variables
         * So they are either a constant (0-ary function) or an n-ary function n>0
         */
        if( strcasecmp(s.getName() , t.getName()) !=0 ) return false;//Symbol clash
        if( isConst(s) ) return true; 
        //They both have the same name therefore they both must be functions of the same arity
        Function* sf = ((Function*) &s);
        Function* tf = ((Function*) &t);
        //Recursively unify s and t
        bool ok;
        for (size_t i = 0; i < sf->getArity(); i++)
        {
            ok = unifyTerm(*sf->getArg(i), *tf->getArg(i),sub);
            if( not ok ) return false;
        }
        return true;
    }
} // namespace Ares
