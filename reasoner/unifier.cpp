#include "reasoner/unifier.hh"
namespace ares
{
    bool Unifier::unifyAtom(const Atom& l1, const Atom& l2, Substitution& sub){
        bool dtName = l1.get_name() != l2.get_name();
        bool dtArity = (l1.arity() != l2.arity() );
        if( dtName || dtArity) 
            return false; //Can't be unified
        
        for (size_t i = 0; i < l1.arity(); i++)
        {
            bool ok = unifyTerm(l1.getArg(i),l2.getArg(i),sub);
            if(not ok) return false; //Can't be unified
        }
        return true;   
    }

    bool Unifier::unifyTerm(const Term* s, const Term* t,Substitution& sub){
        if(s == t)
            return true;
        if( is_var(s) ){
            auto s_v = (Variable*)s;
            if( sub.isBound(s_v) ) 
                return unifyTerm(sub.get(s_v),t,sub);
            //WARNING! No occurs check!
            sub.bind(s_v, t);
            return true;
        }
        //S is not a variable
        if( is_var(t) ) 
            return unifyTerm(t,s,sub);
        /**
         * At this point both s and t are not variables
         * So they are either a constant (0-ary function) or an n-ary function n>0
         */
        if( s->get_name() != t->get_name() ) return false;//Symbol clash
        if( is_const(s) ) return true; 
        //They both have the same name therefore they both must be functions of the same arity
        Function* sf = (Function*) s;
        Function* tf = (Function*) t;
        //Recursively unify s and t
        bool ok;
        for (size_t i = 0; i < sf->arity(); i++)
        {
            ok = unifyTerm(sf->getArg(i), tf->getArg(i),sub);
            if( not ok ) return false;
        }
        return true;
    }
} // namespace ares
