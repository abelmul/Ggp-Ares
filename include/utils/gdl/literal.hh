#ifndef LITERAL_HH
#define LITERAL_HH
#include <string>
#include <vector>
#include "utils/gdl/term.hh"

namespace ares
{
    class ExpressionPool;

    class Literal : public structured_term
    {
    
    friend class ExpressionPool;
    friend class ExpressionPoolTest;

    template<class T>
    friend Body* instantiate(const T& expr,const Substitution &sub,VarSet& vSet, bool fn);

    private:
        Literal(const char* n,bool p,const Body* b,lit_sptr* _this)
        :structured_term(n,p,b,(cnst_term_sptr*)_this,LIT)
        {
        }
        /**
         * Only ExpressionPool could create terms, to ensure only one instance exists 
         */
        void* operator new(std::size_t s);
    public:
        void operator delete(void* p);
        
        virtual ~Literal(){
            name = nullptr;
            if( _body )
                delete _body;
            _body = nullptr;
        }

        Literal(const Literal&) = delete;
        Literal(const Literal&&) = delete;
        Literal& operator= (const Literal&) = delete;
        Literal& operator= (const Literal&&) = delete;

        
        /**
         * Create an instance of this literal do 
         * either in place modifications or by creating
         * a new clone literal and modifying that.
         */
        virtual const cnst_term_sptr operator ()(const Substitution &sub,VarSet& vSet) const;
    };
} // namespace ares

#endif