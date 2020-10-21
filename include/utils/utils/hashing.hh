#ifndef HASHING_HH
#define HASHING_HH
#include "utils/memory/body.hh"

#include <algorithm>
#include <unordered_set>
#include <vector>

namespace ares
{
    class Variable;
    class Term;
    class Query;

    struct CharpHasher {
        std::size_t operator()(const char* name) const;
    };
    struct StrEq {
        bool operator()(const char* s1, const char* s2) const;
    };
    // Used in Substitutions to index using a variable

    struct VarHasher {
        std::size_t operator()(const Variable*) const;
    };

    struct VarEqual {
        bool operator()(const Variable*, const Variable*) const;
    };

    struct ClauseHasher {
        std::size_t operator()(const Clause*) const;
        bool operator()(const Clause* c, const Clause* c2) const;
    };

    struct PoolKey {
        PoolKey(ushort n = 0, const Body* b = nullptr) : name(n), body(b) {}
        ushort name;
        const Body* body = nullptr;
    };

    /**
     * Used to hash and compare variant literals.
     */
    struct AtomHasher {
        std::size_t hash(const Atom*) const;
        bool equal(const Atom*, const Atom*) const;
        inline std::size_t operator()(const Atom* l) const { return hash(l); }
        inline bool operator()(const Atom* l, const Atom* l1) const
        {
            return equal(l, l1);
        }
    };
    /**
     * Hash and compare the id's of queries.
     */
    // struct QueryHasher{
    //     std::size_t operator()(const Query&) const;
    //     bool operator()(const Query&, const Query&) const;
    // };
    /**
     * Hash and compare pool keys of memCache.
     */
    struct PoolKeyHasher {
        std::size_t hash(const PoolKey&) const;
        bool equal(const PoolKey&, const PoolKey&) const;
    };

    struct PoolKeyEqual {
        bool operator()(const PoolKey&, const PoolKey&) const;
    };
    /**
     * Can be iterated like a vector and is random access.
     * But filters duplicates using hashes and equal compare.
     */
    template <class T, class Hash = std::hash<T>, class Eq = std::equal_to<T>>
    class UniqueVector
    {
        typedef typename std::vector<T>::const_iterator iterator;
        typedef std::unordered_set<T, Hash, Eq> set;

     public:
        UniqueVector() = default;

        UniqueVector(std::size_t s)
        {
            elements.reserve(s);
            lookup.reserve(s);
        }
        void share(UniqueVector& vec)
        {
            if (vec.lookupPtr)
                lookupPtr = vec.lookupPtr;
            else
                lookupPtr = &vec.lookup;
        }
        bool push_back(T el)
        {
            // check if el exists
            set* lookup_ = &lookup;
            if (lookupPtr)
                lookup_ = lookupPtr;
            auto it = lookup_->find(el);
            if (it == lookup_->end()) {
                // Doesn't exist, remember it
                lookup_->insert(el);
                elements.push_back(el);
                return true;
            }
            return false;
        }
        void copy_elements(UniqueVector& vec)
        {
            elements.insert(elements.end(), vec.begin(), vec.end());
        }

        inline const std::vector<T>& getElements() const { return elements; }
        inline bool exists(const T& el) const
        {
            const set* lookup_ = &lookup;
            if (lookupPtr)
                lookup_ = lookupPtr;
            if (lookup_->find(el) == lookup_->end())
                return false;

            return true;
        }
        inline void clear()
        {
            elements.resize(0);
            lookup.clear();
        }

        bool operator==(const UniqueVector& other)
        {
            if (elements.size() != other.elements.size())
                return false;
            for (auto&& el : elements)
                if (!other.exists(el))
                    return false;

            return true;
        }
        /**
         * Equivalent to operator== except this considers orders of elements.
         */
        bool equal(const UniqueVector& other)
        {
            if (elements.size() != other.elements.size())
                return false;
            for (size_t i = 0; i < elements.size(); i++)
                if (elements[i] != other.elements[i])
                    return false;

            return true;
        }
        bool operator!=(const UniqueVector& other)
        {
            return not((*this) == other);
        }
        const T& operator[](uint i) const { return elements.at(i); }
        inline std::size_t size() const { return elements.size(); }

        iterator begin() const { return elements.cbegin(); }
        iterator end() const { return elements.cend(); }

        void apply(bool rand,
                   const std::function<void(std::reference_wrapper<const T>)>
                       op) const;

     private:
        std::vector<T> elements;
        set lookup;
        set* lookupPtr = nullptr;
    };
}  // namespace ares

#endif