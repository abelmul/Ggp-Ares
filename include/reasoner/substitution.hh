#ifndef SUBST_HH
#define SUBST_HH

#include "utils/utils/hashing.hh"

#include <functional>
#include <string>
#include <unordered_map>

namespace ares
{
    class Variable;
    class Term;

    typedef std::unordered_map<const Variable*, const Term*, VarHasher,
                               VarEqual>
        Mapping;

    class Substitution
    {
     private:
        Mapping mapping;

     public:
        /**
         * Protect against accidental copying, pass by value, ...
         */
        Substitution& operator=(const Substitution& other) = delete;
        Substitution& operator=(const Substitution&& other) = delete;
        Substitution(const Substitution&& s) = delete;

        Substitution(const Substitution& s)
        {
            this->mapping.insert(s.mapping.begin(), s.mapping.end());
        }

        Substitution() {}
        static Substitution emptySub;

        virtual bool bind(const Variable*, const Term* t);

        // To get the immediate mapping, without traversing the chain.
        virtual const Term* get(const Variable*) const;
        // Overload the indexing operator, to get the underlying exact mapping
        virtual const Term* operator[](const Variable*) const;

        virtual bool isRenaming() const { return false; }
        /**
         * Check if this variable is bound
         */
        virtual bool isBound(const Variable*) const;
        /**
         * Compose this with sub.But this is shallow composition nd need to
         * traverse "chain" to get bound value.
         */
        Substitution* operator+(const Substitution& sub);
        bool isEmpty() const { return mapping.size() == 0; }
        Mapping getMapping() { return mapping; }
        std::string to_string() const;

        virtual ~Substitution() {}
    };

#define EMPTY_SUB Substitution::emptySub
}  // namespace ares
#endif
