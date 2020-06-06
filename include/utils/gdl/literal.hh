#ifndef LITERAL_HH
#define LITERAL_hh
#include <string>
#include <vector>
#include "utils/gdl/term.hh"

namespace Ares
{
    typedef std::vector<Term*>  LitBody;

    class Literal
    {
    private:
        char* name;
        bool positive;
        LitBody* _body;
        LitBody& body;

        Literal(char* n, bool p,uint arity)
        :name(n),positive(p),_body(new LitBody(arity)),body(std::ref(*_body))
        {
        }

        Literal(char* n,bool p,LitBody* b)
        :name(n),positive(p),_body(b),body(std::ref(*_body))
        {
        }
    public:
        Literal(const Literal& l) = delete;
        Literal& operator = (const Literal &l) = delete;

        explicit operator bool() {
            return positive;
        }
        Term* getArg(uint i){
            if( i >= body.size() ) return nullptr;
            
            return body[i];
        }

        uint getArity(){return body.size();}

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
        virtual std::string operator ()(Substitution &sub,VarSet& vSet){
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
        char* getName(){return name;}
        std::string toString(){
            std::string s("(");
            s.append(name);
            for (auto &t : body){
                s.append(" " + t->toString());
            }
            s.append(")");
            return s;
        }
        ~Literal(){
            delete _body;
        }
        friend class GdlParser;

    };
} // namespace Ares

#endif