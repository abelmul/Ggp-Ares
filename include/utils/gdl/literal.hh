#ifndef LITERAL_HH
#define LITERAL_HH
#include <string>
#include <vector>
#include "utils/gdl/term.hh"

namespace Ares
{
    typedef std::vector<Term*>  LitBody;

    class Literal
    {
    
    friend class ExpressionPool;

    private:
        const char* name;
        const bool positive;
        const LitBody* _body;
        const LitBody& body;

        Literal(const char* n, bool p,uint arity)
        :name(n),positive(p),_body(new LitBody(arity)),body(std::ref(*_body))
        {
        }

        Literal(const char* n,bool p,const LitBody* b)
        :name(n),positive(p),_body(b),body(std::ref(*_body))
        {
        }
        /*Managed By ExpressionPool*/
        ~Literal(){
            delete _body;
        }
        
    public:
        Literal(const Literal&) = delete;
        Literal(const Literal&&) = delete;
        Literal& operator= (const Literal&) = delete;
        Literal& operator= (const Literal&&) = delete;

        explicit operator bool() {
            return positive;
        }
        Term* getArg(uint i) const {
            if( i >= body.size() ) return nullptr;
            
            return body[i];
        }

        uint getArity() const {return body.size();}
        
        virtual std::size_t hash() const {
            std::size_t nHash = Term::nameHasher(name);
            for (Term* t : body)
                hash_combine(nHash,t);
            
            return nHash;
        }

        bool isGround(){
            for (Term* arg : body)
                if (!arg->isGround()) return false;
            
            return true;
        }
        /**
         * Create an instance of this literal do 
         * either in place modifications or by creating
         * a new clone literal and modifying that.
         */
        virtual std::string operator ()(Substitution &sub,VarSet& vSet) {
            std::string p("(");
            p.append(name);
            for (size_t i = 0; i < getArity(); i++)
            {
                Term* arg = getArg(i);
                std::string argInst = (*arg)(sub,vSet);
                if( argInst.size() == 0)
                    //Detected loop
                    return std::string();
                p.append( " " + argInst);
            }
            p.append(")");
            return p;
        }
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