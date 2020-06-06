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
        Function(char* name,uint arity):
        Term(name,true),_body(new FnBody(arity)),body(ref(*_body))
        {
            type = FN;
        }

    public:
        //create an initialized function
        Function(char* name,FnBody* _b)
        :Term(name,true),_body(_b),body(ref(*_body))
        {
            type = FN;
        }
        
        uint getArity() const{
            return body.size();
        }
        Term* getArg(uint i)const{
            if( i >= body.size() ) return nullptr;

            return body[i];
        }
        virtual bool isGround(){
            for (Term* arg : body)
                if (!arg->isGround()) return false;
            
            return true;
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
        virtual bool operator==(Term& t)const {
            if(t.getType() != this->type) return false;
            if( strcasecmp(name, t.getName()) != 0 ) return false;
            
            Function* tf = (Function *) &t;
            bool eq = true;
            for (uint i=0;i  < this->getArity() ; i++)
                eq &= ((*getArg(i)) == (*tf->getArg(i)));
            
            return eq;
        }
        virtual std::string toString(){
            std::string s(name);
            std::ostringstream stringStream;
            s.append("(");
            std::string sep ="";
            for (auto &t : body){
                s.append(sep + t->toString());
                sep = ",";
            }
            stringStream << ")";
            #if DEBUG_ARES
            stringStream << "[" << this <<"]";
            #endif
            s.append(stringStream.str());
            return s;
        }
        /**
         * Variables and constants are not deleteable
         * Only one instance of a variable (resp. a constant) exits
         * and they are managed by the gdlPool
         */
        ~Function(){
            for (auto &t : body) 
                if(t->deleteable)
                    delete t;

            delete _body;
            _body = nullptr;
        }
    };
} // namespace Ares


#endif