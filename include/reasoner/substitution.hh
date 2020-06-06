#ifndef SUBST_HH
#define SUBST_HH

#include <unordered_map>
#include <functional>

namespace Ares
{

    class Variable;
    class Term;
    //Used in Substitutions to index using a variable
    struct VarHasher
    {
        std::size_t operator() (const Variable* x) const;
    };
    struct VarEqual
    {
        bool operator()(const Variable *v1, const Variable *v2) const;
    };
    typedef std::unordered_map<Variable*,Term*,VarHasher,VarEqual> Mapping;

    class Substitution
    {

    private:
        Mapping mappping;

    public:
        /**
         * Protect against accidental copying, pass by value, ...
         */
        Substitution(const Substitution& s) = delete;
        Substitution& operator = (const Substitution& other) = delete;

        Substitution(){}
        static Substitution emptySub;

        bool bind(Variable* x, Term* t);
        //Overload the indexing operator, to get the underlying mapping        
        Term* operator[](Variable* x);

        /**
         * Check if this variable is bound
         */ 
        bool isBound(Variable* x);
        /**
         * Overload the += operator for substitution composition.
         * θ:= {X1/s1,...,Xm/sm}
         * σ:= {Y1/t1,...,Yn/tn}
         * Then the composition θσ is:
         * θσ= {X1/s1σ,...,Xm/smσ,Y1/t1,...,Yk/tn} 
         * k<=n, where Each Yi distinct from Xj 
         */
        void operator +=(Substitution& sub);
        Mapping getMapping(){return mappping;}
        ~Substitution();
    };
    
    #define EMPTY_SUB Substitution::emptySub
    typedef Substitution Context;
} // namespace Ares
#endif 
