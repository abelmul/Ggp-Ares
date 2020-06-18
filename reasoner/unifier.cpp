#include "reasoner/unifier.hh"
namespace ares
{
    bool Unifier::unifyPredicate(const cnst_lit_sptr& l1, const cnst_lit_sptr& l2, Substitution& sub){
        bool dtName = strcasecmp(l1->get_name(),l2->get_name()) != 0;
        bool dtSign = ( (bool) *l1 ) ^ (bool) *l2;
        bool dtArity = (l1->getArity() != l2->getArity() );
        if( dtName || dtSign || dtArity) 
            return false; //Can't be unified
        
        for (size_t i = 0; i < l1->getArity(); i++)
        {
            bool ok = unifyTerm(l1->getArg(i),l2->getArg(i),sub);
            if(not ok) return false; //Can't be unified
        }
        return true;   
    }

    bool Unifier::unifyTerm(cnst_term_sptr s, cnst_term_sptr t,Substitution& sub){
        if(s == t)
            return true;
        if( is_var(s) ){
            cnst_var_sptr& s_v = *(cnst_var_sptr*)&s;
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
        if( strcasecmp(s->get_name() , t->get_name()) !=0 ) return false;//Symbol clash
        if( is_const(s) ) return true; 
        //They both have the same name therefore they both must be functions of the same arity
        Function* sf = ((Function*) s.get());
        Function* tf = ((Function*) t.get());
        //Recursively unify s and t
        bool ok;
        for (size_t i = 0; i < sf->getArity(); i++)
        {
            ok = unifyTerm(sf->getArg(i), tf->getArg(i),sub);
            if( not ok ) return false;
        }
        return true;
    }
} // namespace ares
