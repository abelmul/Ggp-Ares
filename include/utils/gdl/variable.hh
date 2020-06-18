#ifndef VARIABLE_HH
#define VARIABLE_HH

#include "utils/gdl/term.hh"

namespace ares
{
    class Variable : public Term
    {
    
    friend class ExpressionPool;
    friend class ExpressionPoolTest;

    private:
        /**
         * Only ExpressionPool could create terms, to ensure only one instance exists 
         */
        Variable(const char* name,cnst_var_sptr* _this):Term(name,(cnst_term_sptr*)_this,VAR)
        {
        }
    public:
        /**
         * Deleting a variable does nothing.
         * The Memory pool will free the malloc'd memory.
         */
        ~Variable(){}
        void operator delete(void* p){}
        /**
         * Apply the Substitution sub on this variable, creating an instance.
         * This is done by traversing the "chain" present within the substitution,
         * vset is used to detect any loops. if a variable is encountered more than once 
         * while traversing a chain then there is a loop.
         */
        virtual const cnst_term_sptr operator ()(const Substitution &sub,VarSet& vSet) const {
            if( not sub.isBound(*(cnst_var_sptr*)_this) ) return *_this;
            else if(sub.isRenaming() ) return sub.get(*(cnst_var_sptr*)_this);        //No need to traverse the chain

            if( vSet.find(*(cnst_var_sptr*)_this) != vSet.end() ) return null_term_sptr;     //There is a circular dependency
            // //Remember this var in this particular path
            vSet.insert(*(cnst_var_sptr*)_this);
            const cnst_term_sptr& t = sub.get(*(cnst_var_sptr*)_this);
            const cnst_term_sptr& tInst = (*t)(sub,vSet);
            // //Done
            vSet.erase(*(cnst_var_sptr*)_this);   
            return tInst;
        }
        virtual bool is_ground() const{
            return false;
        }

        virtual std::size_t hash() const{
            return nameHasher(name);
        }

        virtual std::string to_string() const {
            return std::string(name);
        }
    };
    
} // namespace ares

#endif // VARIABLE_HH
