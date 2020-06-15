#include "ares.hh"

namespace Ares
{
    class ExpressionPoolTest
    {
    private:
        /* data */
    public:
        ExpressionPoolTest(/* args */);
        ~ExpressionPoolTest();
        static void test(){
            auto pool = new ExpressionPool();
            char* names[]= {strdup("y"),strdup("x"),strdup("a"),strdup("p"),strdup("f"),strdup("g")};
            Variable* y = new Variable(names[0]);
            Variable* x = new Variable(names[1]);
            Constant* c = new Constant(names[2]);
            Function* f1 = new Function(names[4],new Body{x,y});
            f1->pool = pool;
            Function* g = new Function(names[5],new Body{y});
            g->pool = pool;
            Function* f2 = new Function(names[4],new Body{g,c});
            f2->pool = pool;

            Literal* p = new Literal(names[3],true,new Body{f1,c});
            p->pool = pool;

            Literal* q = new Literal(names[3],true,new Body{f2,y});
            q->pool = pool;

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
                auto pi = (*p)(sub,vSet) ;
                auto qi = (*q)(sub,vSet2) ;
                if( not pi or (not qi)){
                    cout << "Nulllptr " << endl;
                    fflush(NULL);
                    return;
                }
                std::cout << *(*p)(sub,vSet) << std::endl;
                std::cout << *(*q)(sub,vSet2) << std::endl;
            }
            else std::cout << "Not Unifiable \n";
            // delete _sub;
            // // delete p;
            // // delete q;
            // // delete x;
            // // delete y;
            // // delete c;
            // // delete f1;
            // // delete g ;
            // // delete f2;
            // for (char *i : names)
            //     free(i);
            
        }
    };
    
    ExpressionPoolTest::ExpressionPoolTest(/* args */)
    {
    }
    
    ExpressionPoolTest::~ExpressionPoolTest()
    {
    }
    
} // namespace Ares

int main(int argc, char const *argv[])
{
    using namespace Ares;
    ExpressionPoolTest::test();
    return 0;
}