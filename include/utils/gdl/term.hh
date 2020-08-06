#ifndef TERM_HH
#define TERM_HH

#include <string>
#include <string.h>
#include <sstream>
#include <unordered_set>
#include <stack>
#include <iostream>
#include "reasoner/substitution.hh"
#include "utils/utils/hashing.hh"
#include "utils/memory/memoryPool.hh"
#include "utils/utils/cfg.hh"
#include "utils/memory/namer.hh"
namespace ares
{

    /**
     * TODO: Pre-Compute Ground.
     */
    enum Type {VAR,CONST,FN,LIT};
    class ExpressionPool;
    typedef std::unordered_set<cnst_var_sptr,SpVarHasher,SpVarEqual> VarSet;

    //Variables, functions, constants all inherit from this abstract class.
    class Term
    {
    friend class ExpressionPool;
    
    protected:
        ushort name;
        /*_this is a reference to the shared_ptr in the expression pool.
          Used to quickly reset/delete it in the expression pool.*/
        // cnst_term_sptr* _this = nullptr;
        /**
         * TODO: pre-computed ground value 
         */
        bool ground;    
        Type type;        

        Term(ushort n, Type t):name(n),type(t){}
        virtual ~Term(){}

    public:
        static CharpHasher nameHasher;  
        static cnst_term_sptr null_term_sptr;
        static cnst_lit_sptr null_literal_sptr;
        /**
         * Use Term.operator()(Substitution sub) to create a deep clone.
         * Protect against accidental copying,assignment, and return by value.
        */
        Term(const Term&) = delete;
        Term(const Term&&) = delete;
        Term& operator=(Term&&) = delete;
        Term& operator = (const Term&) = delete;
        /**
         * Apply the Substitution sub on this term, creating an instance.
         * This is done by traversing the "chain" present within the substitution,
         * Varset is used to detect any loops. if a variable is encountered more than once then
         * there is a loop.
         */
        virtual const cnst_term_sptr operator ()(const Substitution &sub,VarSet& vSet) const = 0;

        virtual bool is_ground() const = 0;
        virtual std::size_t hash() const = 0;

        virtual bool operator==(const Term& t) const{
            //They are equal iff they have the same address.
            //Only one instance of a term exists.
            return this == &t;
        };
        virtual bool operator!=(const Term& t) const{
            //They are equal iff they have the same address.
            //Only one instance of a term exists.
            return this != &t;
        };


        ushort get_name() const {return name;}
        Type get_type() const {return type;}
            
        virtual std::string to_string() const = 0;
        friend std::ostream & operator << (std::ostream &out, const Term &t){
            out << t.to_string();
            return out;
        }
    };

    template<class T>
    inline void hash_combine(std::size_t& seed,const T& v)
    {
        std::size_t hash = v->hash();
        seed ^= hash + 0x9e3779b9 + (seed<<6) + (seed>>2);
    }

    
    class structured_term : public Term
    {
    friend class ExpressionPool;
    friend class visualizer;
    protected:
        mutable SpinLock slk;
        mutable bool positive;
        const Body* _body = nullptr;
        const Body& body;

    public:
        structured_term(ushort n, bool p,const Body* b,Type t)
        :Term(n,t),positive(p),_body(b),body(std::ref(*_body))
        {
        }

        /**
         * @param op should have signature of TR op();
         * This just does return op() in a thread safe way.
         * op should be a very fast operation as SpinLock is 
         * used for locking.
         */
        template<class T,class TR>
        TR synchronized(T op) const {
            std::lock_guard<SpinLock> lk(slk);
            return op();
        }
        virtual explicit operator bool() const {
            return positive;
        }
        virtual cnst_term_sptr& getArg(uint i) const {
            if( i >= body.size() ) throw IndexOutOfRange("Structured Term GetArg. Size is " +std::to_string(body.size()) + ", index is " + std::to_string(i)) ;
            
            return body[i];
        }
        virtual const Body& getBody() const { return body;}
        
        virtual uint getArity() const {return body.size();}

        virtual std::size_t hash() const {
            std::size_t nHash = std::hash<ushort>()(name);
            for (const cnst_term_sptr& t : body)
                hash_combine(nHash,t.get());
            
            return nHash;
        }
        virtual bool is_ground() const {
            for (const cnst_term_sptr& arg : body)
                if (!arg->is_ground()) return false;

            return true;
        }
        virtual std::string to_string() const {
            std::string s("(");
            if(not positive) s.append("not ( ");
            s.append(Namer::name(name));
            for (auto &t : body){
                s.append(" " + t->to_string());
            }
            if(not positive) s.append(" )");
            s.append(")");
            return s;
        }
        virtual const cnst_term_sptr operator ()(const Substitution &sub,VarSet& vSet) const = 0;
        
        virtual ~structured_term() {}
    };
    

    #define is_var(t)  (t->get_type() == VAR)
    #define is_const(t)  (t->get_type() == CONST)
    #define is_fn(t)  (t->get_type() == FN)
    #define is_lit(t) (t->get_type() == LIT)
    #define is_struct_term(t) ( is_lit(t) || is_fn(t) )
} // namespace ares

#endif  