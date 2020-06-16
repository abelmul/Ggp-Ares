#include "ares.hh"
#include <chrono> 
using namespace std::chrono;

int main(int argc, char const *argv[])
{
    using namespace Ares;
    Cfg cfg("./ares.cfg.json");
    GdlParser* p = GdlParser::getParser(cfg.parserThreads);
    Game* kb = new Game();
    p->parse(kb,cfg.gdl);
    
    // PrgetProver(KnowledgeBase* _kb,ushort proverThread, ushort negThreads)
    Prover* prover = Prover::getProver(kb, cfg.parserThreads, cfg.negThreads);
    auto exp = p->getExpressionPool();
    auto x = exp->getVar("?x");
    auto b = new Body{x};
    PoolKey key{"init", b, true };
    bool exists ;
    const Literal* init = exp->getLiteral(key,exists);
    auto _cb = [&](const Substitution& sub){
        auto vs = VarSet();
        auto t = (*init)(sub, vs);
        cout << *t << endl;
    };
    CallBack<decltype(_cb)> cb(_cb);
    bool done = false;
    //     // Query(Clause* _g,const  State* _c , const CallBack<T>& _cb, const bool _one,bool& d)
    auto goal = new Clause(nullptr, new ClauseBody{init});
    Query<decltype(_cb)> q(goal, new State(), cb, false, done);
    prover->prove(q);
    return 0;
}
