#ifndef LITERAL_HH
#define LITERAL_HH
#include "utils/gdl/term.hh"

#include <string>
#include <vector>

namespace ares
{
    class MemCache;

    class Atom : public structured_term
    {
        friend class MemCache;
        friend class visualizer;

     private:
        Atom(ushort n, const Body* b) : structured_term(n, b, LIT) {}
        /**
         * Only MemCache could create terms, to ensure only one instance exists
         */
        void* operator new(std::size_t s);
        void operator delete(void* p);

        virtual ~Atom()
        {
            if (body)
                delete body;
            body = nullptr;
        }

     public:
        Atom(const Atom&) = delete;
        Atom(const Atom&&) = delete;
        Atom& operator=(const Atom&) = delete;
        Atom& operator=(const Atom&&) = delete;

        /**
         * Create an instance of this literal do
         * either in place modifications or by creating
         * a new clone literal and modifying that.
         */
        virtual const Term* operator()(const Substitution& sub,
                                       VarSet& vSet) const;
    };
}  // namespace ares

#endif