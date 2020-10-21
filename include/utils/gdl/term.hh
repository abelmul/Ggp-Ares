#ifndef TERM_HH
#define TERM_HH

#include "reasoner/substitution.hh"
#include "utils/memory/memoryPool.hh"
#include "utils/memory/namer.hh"
#include "utils/utils/cfg.hh"
#include "utils/utils/hashing.hh"

#include <string.h>

#include <iostream>
#include <sstream>
#include <stack>
#include <string>
#include <unordered_set>
namespace ares
{
    class MemCache;
    typedef std::unordered_set<const Variable*, VarHasher, VarEqual> VarSet;
    typedef std::unordered_map<ushort, ushort> VarRenaming;

    // Variables, functions, constants all inherit from this abstract class.
    class Term
    {
        friend class MemCache;

     public:
        enum Type { VAR, CONST, FN, LIT, NOT, OR };

     protected:
        ushort name;
        Type type;

        Term(ushort n, Type t) : name(n), type(t) {}
        virtual ~Term() {}

     public:
        /**
         * Use Term.operator()(Substitution sub) to create a deep clone.
         * Protect against accidental copying,assignment, and return by value.
         */
        Term(const Term&) = delete;
        Term(const Term&&) = delete;
        Term& operator=(Term&&) = delete;
        Term& operator=(const Term&) = delete;
        /**
         * Apply the Substitution sub on this term, creating an instance.
         * This is done by traversing the "chain" present within the
         * substitution, Varset is used to detect any loops. if a variable is
         * encountered more than once then there is a loop.
         */
        virtual const Term* operator()(const Substitution& sub,
                                       VarSet& vSet) const = 0;

        virtual bool is_ground() const = 0;
        virtual std::size_t hash() const = 0;
        virtual std::size_t hash(VarRenaming& renaming, ushort& nxt) const = 0;

        virtual bool operator==(const Term& t) const
        {
            // They are equal iff they have the same address.
            // Only one instance of a term exists.
            return this == &t;
        };
        virtual bool operator!=(const Term& t) const { return this != &t; };
        virtual bool equals(const Term& t, VarRenaming& renaming) const = 0;

        ushort get_name() const { return name; }
        Type get_type() const { return type; }

        virtual std::string to_string() const = 0;
        friend std::ostream& operator<<(std::ostream& out, const Term& t)
        {
            out << t.to_string();
            return out;
        }
    };

    template <class T>
    inline void hash_combine(std::size_t& seed, const T& v)
    {
        std::size_t hash = v->hash();
        seed ^= hash + 0x9e3779b9 + (seed << 6) + (seed >> 2);
    }

    class structured_term : public Term
    {
        friend class MemCache;
        friend class visualizer;

     protected:
        mutable SpinLock slk;
        const Body* body = nullptr;
        mutable struct {
            std::atomic_bool value;
            std::atomic_bool valid;
            operator bool() const { return value; }
        } ground;

     public:
        structured_term(ushort n, const Body* b, Type t)
            : Term(n, t), body(b), ground{false, false}
        {
        }

        /**
         * @param op should have signature of TR op();
         * This just does return op() in a thread safe way.
         * op should be a very fast operation as SpinLock is
         * used for locking.
         */
        template <class T, class TR>
        TR synchronized(T op) const
        {
            std::lock_guard<SpinLock> lk(slk);
            return op();
        }
        virtual const Term*& getArg(uint i) const
        {
            if (i >= body->size())
                throw IndexOutOfRange("Structured Term GetArg. Size is " +
                                      std::to_string(body->size()) +
                                      ", index is " + std::to_string(i));

            return (*body)[i];
        }
        virtual const Body& getBody() const { return (*body); }

        virtual uint arity() const { return body->size(); }
        /**
         * Check if this structured term is a variant of term
         * @param t another term to compare against
         * @returns true iff t is a variant of this structured term.
         */
        virtual bool equals(const Term& t, VarRenaming& renaming) const
        {
            if (type != t.get_type() || name != t.get_name())
                return false;
            auto* st = (structured_term*)&t;
            if (arity() != st->arity())
                return false;
            for (size_t i = 0; i < body->size(); i++)
                if (not(*body)[i]->equals(*(*st->body)[i], renaming))
                    return false;
            return true;
        }
        virtual std::size_t hash() const
        {
            std::size_t nHash = std::hash<ushort>()(name);
            for (auto& t : *body) hash_combine(nHash, t);

            return nHash;
        }
        /**
         * This hash function cosiders variants the same.
         */
        virtual std::size_t hash(VarRenaming& renaming, ushort& nxt) const
        {
            std::size_t nHash = std::hash<ushort>()(name);
            for (auto& t : *body)
                nHash ^= t->hash(renaming, nxt) + 0x9e3779b9 + (nHash << 6) +
                         (nHash >> 2);

            return nHash;
        }
        virtual bool is_ground() const
        {
            if (not ground.valid) {
                ground.value = true;
                for (auto& arg : *body)
                    if (!arg->is_ground()) {
                        ground.value = false;
                        break;
                    }

                ground.valid = true;
            }
            return ground;
        }
        virtual std::string to_string() const
        {
            std::string s;
            if (body->size())
                s.append("(");
            s.append(Namer::name(name));
            for (auto& t : *body) { s.append(" " + t->to_string()); }
            if (body->size())
                s.append(")");
            return s;
        }

        virtual const Term* operator()(const Substitution& sub,
                                       VarSet& vSet) const = 0;

        virtual ~structured_term() {}
    };

#define is_var(t) (t->get_type() == Term::VAR)
#define is_const(t) (t->get_type() == Term::CONST)
#define is_fn(t) (t->get_type() == Term::FN)
#define is_lit(t) (t->get_type() == Term::LIT)
#define is_struct_term(t) (is_lit(t) || is_fn(t))
}  // namespace ares

#endif