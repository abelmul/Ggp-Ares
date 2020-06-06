#include "ares.hh"

namespace Ares
{
    class GdlParser
    {
    private:
        /* data */
    public:
        GdlParser(/* args */);
        ~GdlParser();
        static void test(){
            char* names[]= {strdup("y"),strdup("x"),strdup("a"),strdup("p"),strdup("f"),strdup("g")};
            Variable* y = new Variable(names[0]);
            Variable* x = new Variable(names[1]);
            Constant* c = new Constant(names[2]);
            Function* f1 = new Function(names[4],new FnBody{x,y});
            Function* g = new Function(names[5],new FnBody{y});
            Function* f2 = new Function(names[4],new FnBody{g,c});

            Literal* p = new Literal(names[3],true,new LitBody{f1,c});
            Literal* q = new Literal(names[3],true,new LitBody{f2,y});
                                
            std::cout << p->toString() << std::endl;
            std::cout << q->toString() << std::endl;
            fflush(NULL);
            Substitution* _sub = new Substitution;
            Substitution& sub = std::ref(*_sub);
            auto ok = Unifier::unifyPredicate(*p,*q,sub);
            if( ok ){
                std::cout << "MGU: \n" ;
                for (auto &it : sub.getMapping())
                    std::cout << it.first->toString() << " |--->" << it.second->toString() << std::endl;

                VarSet vSet,vSet2;
                std::cout << (*p)(sub,vSet) << std::endl;
                std::cout << (*q)(sub,vSet2) << std::endl;
            }
            else std::cout << "Not Unifiable \n";
            delete _sub;
            delete p;
            delete q;
            delete x;
            delete y;
            delete c;
            delete f1;
            delete g ;
            delete f2;
            for (char *i : names)
                free(i);
            
        }
    };
    
    GdlParser::GdlParser(/* args */)
    {
    }
    
    GdlParser::~GdlParser()
    {
    }
    
} // namespace Ares

int main(int argc, char const *argv[])
{
    using namespace Ares;
    GdlParser::test();
    return 0;
}