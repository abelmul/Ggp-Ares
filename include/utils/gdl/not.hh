#ifndef GDL_NOT_HH
#define GDL_NOT_HH

#include "term.hh"
namespace ares
{
    class Not :public structured_term
    {
    public:
        Not(const Body* body)
        :structured_term(Namer::NOT,body,Term::NOT) 
        {}
        virtual const Term* operator ()(const Substitution &sub,VarSet& vSet) const;

    private:
            /**
         * Only MemCache could create terms, to ensure only one instance exists 
         */
        void* operator new(std::size_t s);
        void operator delete(void* p);
        ~Not() {
            if(body)
                delete body;
            body = nullptr;
        }

    friend class MemCache;
    };
} // namespace ares

#endif