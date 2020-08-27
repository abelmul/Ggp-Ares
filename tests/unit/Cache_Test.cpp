#include "common.hh"
#include "reasoner/cache.hh"
#include <unordered_set>

using namespace ares;
void setup();
void Hashing_Correct();
void Hashing_Incorrect();
void AnswerIterator();
void Hashing_Variants();
void CacheTest();
ares::Cfg ares::cfg("./ares.cfg.json");
namespace ares{
    std::atomic<int> Query::nextId = 0;
    AnsIterator Cache::NOT_CACHED(nullptr,-1,nullptr);
}
int main(int argc, char const *argv[])
{
    setup();
    Runner runner;
    runner.iter = 100;
    add_test(runner,Hashing_Correct);
    add_test(runner,Hashing_Incorrect);
    add_test(runner,Hashing_Variants);
    add_test(runner,CacheTest);
    runner();
    return 0;
}
void setup(){
    srand(time(NULL));
    Ares ares;
    ares.mempool = new MemoryPool(100,100,std::vector<std::pair<arity_t,uint>>());
    ares.memCache = ares.mempool->getCache();
    Body::mempool = ClauseBody::mempool = ares.mempool;
    Term::null_term_sptr = nullptr;
    Term::null_literal_sptr = nullptr;
}

/**
 * Variant atoms should hash the same and also be equal.
 * let A1 be an atom and θ := {x0/0, ..., xn/n} where xi is the ith distict variable
 * to occur when counting from the left. The 
 * LiteralHasher::hash(A1) == A1(θ).hash() and also
 * A1.equals( A1(θ) )
 */
void Hashing_Correct(){
    ushort depth = (rand() % 2) +1;
    ushort max_arity = (rand() % 10) + 10;
    
    //Get a random literal, and the variables that occur within it ordered by place of first occurence. 
    auto [lit, vars] = getRandLiteral(depth,max_arity,3);

    //Create a renaming, θ := {x0/0, ..., xn/n}
    VariantSubstitution theta;
    VarRenaming renaming;

    uint i=0;
    for (auto &&v : vars)
        theta.bind(v, cnst_term_sptr( Ares::memCache->getVar(i++)) );

    VarSet vset;
    cnst_term_sptr renamed = (*lit)(theta,vset);

    /**
     * Positive tests, against the base renaming θ := {x0/0, ..., xn/n}
     */
    //Compare the hashes
    assert_true( (LiteralHasher()(lit) == renamed->hash()) );
    assert_true( (LiteralHasher()(*(cnst_lit_sptr*)&renamed) == LiteralHasher()(lit)) );
    //Should be equal
    assert_true( (lit->equals(*renamed.get(),renaming)) );
    renaming.clear();
    assert_true( (lit->equals(*lit.get(),renaming)) );
    renaming.clear();
    assert_true( (renamed->equals(*lit.get(),renaming)) );
    renaming.clear();
    assert_true( (renamed->equals(*renamed.get(),renaming)) );
}
/**
 * Variant atoms should hash the same and also be equal.
 * let A1 be an atom and θ := {x0/0, ..., xn/n} where xi is the ith distict variable
 * to occur when counting from the left. The 
 * LiteralHasher::hash(A1) == A1(θ).hash() and also
 * A1.equals( A1(θ) )
 */
void Hashing_Incorrect(){
    /**
     * Negative tests 
     */
    ushort depth = (rand() % 2) +1;
    ushort max_arity = (rand() % 10) + 10;
    
    VariantSubstitution sigma;
    uint i=0;
    cnst_lit_sptr lit;
    OrdrdVarSet vars;
    do
    {
        auto [l, v] = getRandLiteral(depth,max_arity,3);
        lit = l; vars = v;
    } while (lit->is_ground());
    
    VarRenaming renaming;

    uint change;
    if( vars.size() > 0)
        change = rand() % vars.size();
    
    for (auto &&v : vars){
        if( i == change ){
            sigma.bind(v, cnst_term_sptr( getRandConst(vars,depth) ));
            continue;
        } 
        sigma.bind(v, cnst_term_sptr( Ares::memCache->getVar(i++)) );
    }
    VarSet vset;
    cnst_term_sptr renamed = (*lit)(sigma,vset);
    
    //Compare the hashes
    renaming.clear();
    assert_false(( (LiteralHasher()(lit) == renamed->hash()) and lit->equals(*renamed.get(),renaming) )) ;
    renaming.clear();
    assert_false( ( (LiteralHasher()(*(cnst_lit_sptr*)&renamed) == LiteralHasher()(lit)) and lit->equals(*renamed.get(),renaming) ));
    //Should not be equal
    renaming.clear();
    assert_false( (lit->equals(*renamed.get(),renaming)) );
    renaming.clear();
    assert_false( (renamed->equals(*lit.get(),renaming)) );
    
}
/**
 * Variant atoms should hash the same and also be equal.
 * let A1 be an atom and θ := {x0/y1, ..., xn/yn} then
 * LiteralHasher::hash(A1) == A1(θ).hash() and also
 * A1.equals( A1(θ) )
 */
void Hashing_Variants(){
    ushort depth = (rand() % 2) +1;
    ushort max_arity = (rand() % 10) + 10;
    
    VariantSubstitution sigma;
    auto [lit, vars] = getRandLiteral(depth,max_arity,3);
    VarRenaming renaming;
    //Create a random renaming   
    OrdrdVarSet varsnew;
    std::unordered_set<const Variable*> seen;
    for (auto &&v : vars){
        cnst_term_sptr vp = getRandVar(varsnew,depth);
        while (seen.find((Variable*)vp.get()) != seen.end()){
            vp = getRandVar(varsnew,depth);
        }
        sigma.bind(v, vp);
        seen.insert((Variable*)vp.get());
    }
    
    VarSet vset;
    cnst_term_sptr renamed = (*lit)(sigma,vset);
    //Compare the hashes
    assert_true( (LiteralHasher()(lit) == LiteralHasher()(*(cnst_lit_sptr*)&renamed)) );
    assert_true( (LiteralHasher()(*(cnst_lit_sptr*)&renamed) == LiteralHasher()(lit)) );
    //Should be equal
    assert_true( (lit->equals(*renamed.get(),renaming)) );
    renaming.clear();
    assert_true( (renamed->equals(*lit.get(),renaming)) );
}

void CacheTest(){

    std::vector<uint> queries;
    std::vector<Query> newQueries;
    Query::resetid();
    uint qcount=0;
    auto assertQadded =[&](const Query& q){
        qcount++;
        assert( std::find(queries.begin(),queries.end(),q.id ) != queries.end() );
    };
    Cache cache;
    //Create a solution node
    auto [lit, vars] = getRandLiteral(3,15);
    while (lit->is_ground()){
        auto [l,v] = getRandLiteral(3,15);
        lit = l; vars =v;
    }
    auto clause = getRandClause();
    while( clause->size() == 0)
        clause = getRandClause();
        
    std::atomic_bool done;
    std::shared_ptr<CallBack> cb(new ClauseCBOne(done,nullptr));
    Query q(clause,cb,nullptr);
    q->front() = lit;

    assert_true( cache[q] == Cache::NOT_CACHED );
    
    //insert solutions
    uint nSoln = (rand() % 10) + 1;
    std::unordered_set<ushort> seenConst;
    OrdrdVarSet dummy;
    ushort d;

    auto addAns = [&,lit(lit),vars(vars)](){
        for (size_t i = 0; i < nSoln; i++)
        {
            //just bind the first variable to 
            bool added =false;
            do
            {
                auto c = getRandConst(dummy,d);
                while ( seenConst.find(c->get_name()) != seenConst.end() )
                    c = getRandConst(dummy,d);
                Substitution theta;
                theta.bind( *vars.begin(), c);
                added = cache.addAns(lit, theta);
            } while (!added);   
            assert_true( cache.hasChanged() );
        }
    };
    auto addQueries = [&,lit(lit)](const uint&& soln){
        uint nQ = (rand() % 10)+3;
        for (size_t i = 0; i < nQ; i++)
        {
            auto clause2 = getRandClause();
            while( clause2->size() == 0)
                clause2 = getRandClause();

            Query q2(clause2,cb,nullptr);
            queries.push_back(q2.id);
            q2->front() = lit;
            auto it = cache[q2];
            newQueries.push_back(q2);
            assert_true( it != Cache::NOT_CACHED );
            assert_true( (it.end() - it.begin()) == soln );
        }  
    };
    //there are no solutions.
    addQueries(0);
    addAns();
    //there are no prev solutions, new solutions should not have taken affect now.
    addQueries(0);

    cache.next(assertQadded);
    assert_true( qcount == queries.size() );
    qcount = 0;
    
    //newQueries
    uint nQ = (rand() % newQueries.size());
    auto restart = [&,lit(lit)](const uint& solnExpected){
        queries.clear();
        for (size_t i = 0; i < nQ; i++)
        {
            newQueries[i].goal = getRandClause();
            while(newQueries[i]->size() == 0)
                newQueries[i].goal = getRandClause();
            
            newQueries[i]->front() = lit;
            auto it = cache[newQueries[i]];
            queries.push_back(newQueries[i].id);

            assert_true( it != Cache::NOT_CACHED );
            //Should be able to consume the previous solutions.
            assert_true( (it.end() - it.begin()) == solnExpected );
        }
    };

    //Consume all the answers
    restart(nSoln);
    //Assert no answerlist is queued
    auto assertNoSoln = [](const Query&){assert_true(false);};
    cache.next(assertNoSoln);
    assert_false(cache.hasChanged());
    //No answer should be consumed
    restart(0);
    assert_false(cache.hasChanged());
    //Add new answers
    nSoln = (rand() % 10) + 1;
    addAns();
    // for  (size_t i = 0; i < nQ; i++) {}
    cache.next(assertQadded);

    restart(nSoln);
   
    // assert_true( )
    //Create a lookup node
    //use the solutions
    //insert new solutions
    //use this new solutions
}