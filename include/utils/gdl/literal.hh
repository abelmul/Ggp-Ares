#ifndef LITERAL_HH
#define LITERAL_HH
#include <string>
#include <vector>
#include "utils/gdl/term.hh"

namespace Ares
{
    class ExpressionPool;

    class Literal
    {
    
    friend class ExpressionPool;
    friend class ExpressionPoolTest;

    template<class T>
    friend Body* instantiate(const T& expr,Substitution &sub,VarSet& vSet, bool fn);

    private:
        const char* name;
        const bool positive;
        const Body* _body;
        const Body& body;
        Literal(const char* n, bool p,uint arity)
        :name(n),positive(p),_body(new Body(arity)),body(std::ref(*_body))
        {
        }

        Literal(const char* n,bool p,const Body* b)
        :name(n),positive(p),_body(b),body(std::ref(*_body))
        {
        }
        /*Managed By ExpressionPool*/
        virtual ~Literal(){
            delete _body;
        }
        
    public:
        ExpressionPool* pool;
        Literal(const Literal&) = delete;
        Literal(const Literal&&) = delete;
        Literal& operator= (const Literal&) = delete;
        Literal& operator= (const Literal&&) = delete;

        explicit operator bool() const {
            return positive;
        }
        const Term* getArg(uint i) const {
            if( i >= body.size() ) return nullptr;
            
            return body[i];
        }
        
        const Body* getBody() const { return _body;}
        uint getArity() const {return body.size();}
        
        virtual std::size_t hash() const {
            std::size_t nHash = Term::nameHasher(name);
            for (const Term* t : body)
                hash_combine(nHash,t);
            
            return nHash;
        }

        bool isGround() const {
            for (const Term* arg : body)
                if (!arg->isGround()) return false;

            return true;
        }
        /**
         * Create an instance of this literal do 
         * either in place modifications or by creating
         * a new clone literal and modifying that.
         */
        virtual const Literal* operator ()(Substitution &sub,VarSet& vSet)const ;
        const char* getName() const{return name;}
        std::string toString() const {
            std::string s("(");
            s.append(name);
            for (auto &t : body){
                s.append(" " + t->toString());
            }
            s.append(")");
            return s;
        }
        friend std::ostream & operator << (std::ostream &out, const Literal &l){
            out << l.toString();
            return out;
        }
    };
} // namespace Ares

#endif