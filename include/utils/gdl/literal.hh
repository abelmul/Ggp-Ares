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

    public:
        Literal(const Literal& l) = delete;
        Literal& operator = (const Literal &l) = delete;

        Literal(char* n,bool p,LitBody* b)
        :name(n),positive(p),_body(b),body(std::ref(*_body))
        {
        }

        explicit operator bool() {
            return positive;
        }
        Term* getArg(uint i){
            if( i >= body.size() ) return nullptr;
            return body[i];
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
        char* getName(){return name;}
        ~Literal();
    };
    Literal::~Literal(){
        /**
         * Assuming a term is unique to a literal
         * TODO: Check if a term could be shared between literals
         */ 
        for (auto &t : body)
            if(t->deleteable)
                delete t;

        delete _body;
        delete name;
        free(name);
        _body = nullptr;
    }
} // namespace Ares

#endif