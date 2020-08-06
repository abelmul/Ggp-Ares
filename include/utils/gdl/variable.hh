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
        Variable(ushort name,cnst_var_sptr* _t):Term(name,VAR),_this(_t)
        {
        }
    public:
        cnst_var_sptr* _this = nullptr;
        /**
         * Deleting a variable does nothing.
         * The Memory pool will free the malloc'd memory.
         */
        ~Variable(){}
        void operator delete(void*){}
        /**
         * Apply the Substitution sub on this variable, creating an instance.
         * This is done by traversing the "chain" present within the substitution,
         * vset is used to detect any loops. if a variable is encountered more than once 
         * while traversing a chain then there is a loop.
         */
        virtual const cnst_term_sptr operator ()(const Substitution &sub,VarSet& vSet) const {
            if( not sub.isBound(*_this) ) return *_this;
            else if(sub.isRenaming() ) return sub.get(*_this);        //No need to traverse the chain

            if( vSet.find(*_this) != vSet.end() ) return null_term_sptr;     //There is a circular dependency
            // //Remember this var in this particular path
            vSet.insert(*_this);
            const cnst_term_sptr& t = sub.get(*_this);
            const cnst_term_sptr& tInst = (*t)(sub,vSet);
            // //Done
            vSet.erase(*_this);   
            return tInst;
        }
        virtual bool is_ground() const{
            return false;
        }

        virtual std::size_t hash() const{
            return std::hash<ushort>()(name);
        }

        virtual std::string to_string() const {
            return Namer::vname(name);
        }
    };
    
} // namespace ares

#endif // VARIABLE_HH
