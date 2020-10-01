#ifndef GDL_OR_HH
#define GDL_OR_HH
#include "term.hh"
namespace ares
{
    class Or : public structured_term
    {
        friend class MemCache;
    public:
        Or(const Body* body)
        :structured_term(Namer::OR,body,Term::OR)
        {}

        virtual const Term* operator ()(const Substitution &sub,VarSet& vSet) const;

    private:
        /**
         * Only MemCache could create terms, to ensure only one instance exists 
         */
        void* operator new(std::size_t s);
        void operator delete(void* p);
        ~Or() {
            if(body)
                delete body;
            body = nullptr;
        }
    };
} // namespace ares

#endif