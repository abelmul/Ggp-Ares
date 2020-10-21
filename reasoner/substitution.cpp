#include "reasoner/substitution.hh"
#include "utils/gdl/term.hh"
#include "utils/gdl/variable.hh"

#include <string.h>

namespace ares
{
    Substitution Substitution::emptySub;

    bool Substitution::bind(const Variable* x, const Term* t)
    {
        // This variable has already been bound!
        if (mapping.find(x) != mapping.end())
            return false;
        mapping[x] = t;
        return true;
    }
    const Term* Substitution::get(const Variable* x) const
    {
        return mapping.at(x);
    }
    // Overload the indexing operator, to get the underlying mapping
    const Term* Substitution::operator[](const Variable* x) const
    {
        if (not isBound(x))
            return nullptr;

        VarSet vSet;
        vSet.insert(x);
        const Term* t = mapping.at(x);
        return (*t)(*this, vSet);  // Walk through the implicit `chain`
    }

    bool Substitution::isBound(const Variable* x) const
    {
        return mapping.find(x) != mapping.end();
    }

    Substitution* Substitution::operator+(const Substitution& sub)
    {
        Substitution* sNew = new Substitution();
        sNew->mapping.insert(mapping.begin(), mapping.end());
        sNew->mapping.insert(sub.mapping.begin(), sub.mapping.end());
        return sNew;
    }
    std::string Substitution::to_string() const
    {
        std::string s("{\n");
        for (auto&& i : mapping)
            s.append(i.first->to_string() + " :-> " + i.second->to_string());
        s.append("}\n");
        return s;
    }
}  // namespace ares
