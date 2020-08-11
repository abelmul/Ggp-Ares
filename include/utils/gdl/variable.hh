#ifndef VARIABLE_HH
#define VARIABLE_HH

#include "utils/gdl/term.hh"

namespace ares
{
    class Variable : public Term
    {
    
    friend class MemCache;
    friend class ExpressionPoolTest;

    private:
        /**
         * Only MemCache could create terms, to ensure only one instance exists 
         */
        Variable(ushort name):Term(name,VAR)
        {
        }
    public:
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
            if( not sub.isBound(this) ) return cnst_var_sptr(this,[](const Term*){});
            else if(sub.isRenaming() ) return sub.get(this);        //No need to traverse the chain

            if( vSet.find(this) != vSet.end() ) return null_term_sptr;     //There is a circular dependency
            // //Remember this var in this particular path
            vSet.insert(this);
            const cnst_term_sptr& t = sub.get(this);
            const cnst_term_sptr& tInst = (*t)(sub,vSet);
            // //Done
            vSet.erase(this);   
            return tInst;
        }
        virtual bool is_ground() const{
            return false;
        }
        /**
         * Check if two variables are vairiant with an atom.
         */
        virtual bool equals(const Term& t,VarRenaming& renaming) const {
            if( t.get_type() != VAR) { return false;}
            auto it = renaming.find(name);
            if( it != renaming.end() ){
                return t.get_name() == it->second;
            }
            renaming[name] = t.get_name();
            return true;
        }
        virtual std::size_t hash() const{
            return std::hash<ushort>()(name);
        }
        virtual std::size_t hash(VarRenaming& renaming,ushort& nxt) const {
            auto it = renaming.find(name); 
            if( it != renaming.end()){
                //this variable has been renamed 
                return std::hash<ushort>()(it->second);
            }
            renaming[name] = nxt;
            return std::hash<ushort>()(nxt++);
        }
        virtual std::string to_string() const {
            return Namer::vname(name);
        }
    };
    
} // namespace ares

#endif // VARIABLE_HH
