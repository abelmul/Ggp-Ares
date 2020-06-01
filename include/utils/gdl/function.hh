#ifndef FUNCTION_HH
#define FUNCTION_HH

#include "utils/gdl/term.hh"


namespace Ares
{
    typedef std::vector<Term*> FnBody;
    //This represents gdl functions
    class Function:public Term
    {

    private:
        FnBody* _body = nullptr;
        FnBody& body;

        //Create a function with an empty body, used only during instantiation.
        Function(std::string n,uint arity):
        Term(n,true),_body(new FnBody(arity)),body(ref(*_body))
        {
        }

    public:
        //create an initialized function
        Function(std::string n,FnBody* _b)
        :Term(n,true),_body(_b),body(ref(*_body))
        {
        }
        
        uint getArity(){
            return body.size();
        }
        
        virtual bool isGround(){
            bool ground = true;
            for (Term* arg : body)
                ground &= arg->isGround();
            
            return ground;
        }
        /**
         * Create an instance of this function do 
         * either in place modifications or by creating
         * a new term and modifying that.
         */
        virtual Term* operator ()(Substitution &sub,bool inPlace=false){
            Function* instance = this;

            if(!inPlace)
                instance  = new Function(this->name,getArity());

            for (size_t i = 0; i < this->getArity(); i++)
            {
                Term* arg = this->body[i];
                instance->body[i] = (*arg)(sub,inPlace);
            }
            return instance;
        }

        virtual std::string toString(){
            std::string s = name;
            std::ostringstream stringStream;
            s.append("(");
            std::string sep ="";
            for (auto &t : body){
                s.append(sep + t->toString());
                sep = ",";
            }
            stringStream << ")";
            // stringStream << "[" << this <<"]";
            s.append(stringStream.str());
            return s;
        }
        ~Function();
    };
} // namespace Ares


#endif