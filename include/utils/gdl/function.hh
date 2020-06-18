#ifndef FUNCTION_HH
#define FUNCTION_HH

#include "utils/gdl/term.hh"


namespace ares
{
    /**
     * TODO: Create A pool of FunctionBodies and create a user level cache of bodies
     * managed by ExpressionPool, maybe rename it to MemoryPool.
     */

    //This represents gdl functions
    class Function:public structured_term
    {
    
    friend class ExpressionPool;
    friend class ExpressionPoolTest;

    template<class T>
        friend Body* instantiate(const T& expr,const Substitution &sub,VarSet& vSet, bool fn);
    
    private:
        Function(const Function&) = delete;
        Function(const Function&&) = delete;
        Function& operator=(Function&&) = delete;
        Function& operator = (const Function&) = delete;

        //create an initialized function
        Function(const char* name,const Body* _b,fn_sptr* _this)
        :structured_term(name,true,_b,(cnst_term_sptr*)_this,FN)
        {
        }

        Function(const char* name,bool p, const Body* _b,fn_sptr* _this)
        :Function(name, _b, _this)
        {}
        /**
         * Only ExpressionPool could create terms, to ensure only one instance exists 
         */
        void* operator new(std::size_t s);
    public:
        void operator delete(void* p);

        virtual ~Function(){
            /**
             * Remove the nulling after testing
             * this will be usefull to debug issues related to memory
             * when a structured_term is reused you won't mistakenely
             * use the prev value when its allocated again, it will segfault.
             */
            name = nullptr;
            if(_body)
                delete _body;
            _body = nullptr;
        }
        
        /**
         * Apply the Substitution sub on this term, creating an instance.
         * This is done by traversing the "chain" present within the substitution,
         * Varset is used to detect any loops. if a variable is encountered more than once on a 
         * single dfs path then there is a loop.
         */
        virtual const cnst_term_sptr operator ()(const Substitution &sub,VarSet& vSet) const;
    };
} // namespace ares


#endif