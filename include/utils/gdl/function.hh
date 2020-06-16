#ifndef FUNCTION_HH
#define FUNCTION_HH

#include "utils/gdl/term.hh"


namespace Ares
{
    /**
     * TODO: Create A pool of FunctionBodies and create a user level cache of bodies
     * managed by ExpressionPool, maybe rename it to MemoryPool.
     */

    //This represents gdl functions
    class Function:public Term
    {
    
    friend class ExpressionPool;
    friend class ExpressionPoolTest;

    template<class T>
        friend Body* instantiate(const T& expr,const Substitution &sub,VarSet& vSet, bool fn);
    
    private:
        const Body* _body = nullptr;
        const Body& body;

        //Create a function with an empty body, used only during instantiation.
        Function(const char* name,uint arity):
        Term(name,FN),_body(new Body(arity)),body(ref(*_body))
        {
        }

        //create an initialized function
        Function(const char* name,const Body* _b)
        :Term(name,FN),_body(_b),body(ref(*_body))
        {
        }
        /*Managed By ExpressionPool*/
        virtual ~Function(){
            delete _body;
        }
    public:
        
        uint getArity() const{
            return body.size();
        }
        const Term* getArg(uint i)const{
            if( i >= body.size() ) return nullptr;

            return body[i];
        }
        virtual bool isGround() const {
            for (const Term* arg : body)
                if (!arg->isGround()) return false;
            
            return true;
        }
        virtual std::size_t hash() const {
            std::size_t nHash = nameHasher(name);
            for (const Term* t : body)
                hash_combine(nHash,t);
            
            return nHash;
        }
        /**
         * Apply the Substitution sub on this term, creating an instance.
         * This is done by traversing the "chain" present within the substitution,
         * Varset is used to detect any loops. if a variable is encountered more than once on a 
         * single dfs path then there is a loop.
         */
        virtual const Term* operator ()(const Substitution &sub,VarSet& vSet) const;
        
        virtual std::string toString() const{
            std::string s("(");
            s.append(name);
            for (auto &t : body){
                s.append(" " );
                s.append(t->toString());
            }
            s.append(")");
            return s;   
        }

    };
} // namespace Ares


#endif