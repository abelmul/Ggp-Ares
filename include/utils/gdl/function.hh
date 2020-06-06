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
        Term(name,FN),_body(new FnBody(arity)),body(ref(*_body))
        {
        }

        //create an initialized function
        Function(char* name,FnBody* _b)
        :Term(name,FN),_body(_b),body(ref(*_body))
        {
        }
    public:
        
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
         * Apply the Substitution sub on this term, creating an instance.
         * This is done by traversing the "chain" present within the substitution,
         * Varset is used to detect any loops. if a variable is encountered more than once on a 
         * single dfs path then there is a loop.
         */
        virtual std::string operator ()(Substitution &sub,VarSet& vSet){
            std::string f("(");
            f.append(name);
            for (size_t i = 0; i < getArity(); i++)
            {
                Term* arg = getArg(i);
                std::string argInst = (*arg)(sub,vSet);
                if( argInst.size() == 0)
                    //Detected loop
                    return std::string();
                f.append( " " + argInst);
            }
            f.append(")");
            return f;
        }

        
        virtual std::string toString(){
            std::string s("(");
            s.append(name);
            for (auto &t : body){
                s.append(" " );
                s.append(t->toString());
            }
            s.append(")");
            return s;   
        }
        ~Function(){
            delete _body;
        }
        friend class GdlParser;

    };
} // namespace Ares


#endif