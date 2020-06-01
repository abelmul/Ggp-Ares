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
        std::string name;
        bool positive;
        LitBody* _body;
        LitBody& body;

        Literal(std::string n, bool p,uint arity)
        :name(n),positive(p),_body(new LitBody(arity)),body(std::ref(*_body))
        {
        }

    public:
        Literal(const Literal& l) = delete;
        Literal& operator = (const Literal &l) = delete;

        Literal(std::string n,bool p,LitBody* b)
        :name(n),positive(p),_body(b),body(std::ref(*_body))
        {
        }

        explicit operator bool() {
            return positive;
        }
        uint getArity(){return body.size();}

        bool isGround(){
            bool ground = true;

            for (auto &t : body)
                ground &= t->isGround();
            
            return ground;
        }
        /**
         * Create an instance of this literal do 
         * either in place modifications or by creating
         * a new clone literal and modifying that.
         */
        Literal* operator ()(Substitution sub,bool inPlace=false){
            Literal* instance = this;
            if(!inPlace)
                instance = new Literal(name,positive,getArity());
            
            for (uint i=0;i<instance->getArity();i++)
            {
                Term* arg = body[i];
                instance->body[i] = (*arg)(sub,inPlace);
            }
            return instance;
        }
        ~Literal();
    };
    Literal::~Literal(){
        for (auto &t : body)
            if(t->deleteable)
                delete t;

        delete _body;
        _body = nullptr;
    }
} // namespace Ares

#endif