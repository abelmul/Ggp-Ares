#include "common.hh"
#include "reasoner/cache.hh"
#include <unordered_set>
#include <atomic>
using namespace ares;
void setup();
void AnswerIterator();
void AnsList();
ares::Cfg ares::cfg("./ares.cfg.json");

namespace ares{
    std::atomic<int> Query::nextId = 0;
}
int main(int argc, char const *argv[])
{
    setup();
    Runner runner;
    runner.iter = 100;
    add_test(runner, AnswerIterator);
    add_test(runner, AnsList);
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
 * The answer iterator should take in a next clause, a container, and a ptr--current position.
 * It should iterate over the elements begining from container[current] to conainer.end()
 */
void AnswerIterator(){
    AnsIterator::container elements;
    uint iter = (rand() % 20) + 10;
    for (size_t i = 0; i < iter; i++)
    {
        ushort depth = (rand() % 2) +1;
        ushort max_arity = (rand() % 10) + 10;
        
        //Get a random literal, and the variables that occur within it ordered by place of first occurence. 
        auto [lit, vars] = getRandLiteral(depth,max_arity,3);
        elements.push_back(lit);
    }

    auto asserter = [&elements](AnsIterator& ansit,const uint& curr){
        std::unordered_set<const Literal*> visited;
        for (auto &&l : ansit)
            visited.insert(l.get());
        
        assert_true( (visited.size() == (elements.size() - curr) ) );
        uint i=0;
        for (auto &&l : elements)
        {
            if( i < curr){//Nothing below curr should be visted
                assert_true( (visited.find(l.get()) == visited.end() ));
            }
            else//Everything above and including curr should be visted
                assert_true( visited.find(l.get()) != visited.end());
            i++;
        }
    };
    AnsIterator ansitBegin(&elements,0,nullptr);
    AnsIterator ansitBEnd(&elements,elements.size()-1,nullptr);
    AnsIterator ansitAEnd(&elements,elements.size(),nullptr);
    uint curr = rand() % elements.size();
    AnsIterator ansitRand(&elements,curr,nullptr);

    asserter(ansitBegin, 0);
    asserter(ansitBEnd, elements.size()-1);
    asserter(ansitAEnd, elements.size());
    asserter(ansitRand, curr);
}

void AnsList(){
    AnswerList ansList;
    std::atomic_bool done;
    std::shared_ptr<CallBack> cb(new ClauseCBOne(done, nullptr));

    std::vector<Query> queries;
    uint nqueries = (rand() % 10) + 5;
    
    auto accummulate =[&](Query& q){
        queries.push_back(std::move(q));
    };
    auto asserter = [&](AnsIterator& it, Clause* cl){
        auto& body = cl->getBody();
        for (size_t i = 1; i < body.size(); i++)
            assert_true( (it.nxt->getBody()[i-1] == body[i]) );
    };
    auto insertQueries = [&](ushort n){
        nqueries = (rand() % 10) + 5;
        for (size_t i = 0; i < nqueries; i++)
        {
            auto c = getRandClause();
            auto* clause = c.get();
            Query q(c, cb, nullptr);
            auto it = ansList.addObserver(std::move(q));
            assert_true( (it.end() - it.begin()) == n );
            assert_false(q.goal);   // Goal should be moved
            asserter(it, clause);
        }
    };
    insertQueries(0);
    auto [sizeq, size2] = ansList.sizeob();
    assert_true( (sizeq == nqueries and size2 == 0 ));
    ansList.next();
    ansList.apply([](Query&){});
    auto [size, size1] = ansList.sizeob();
    assert_true( (size == 0 and size1 == 0 ));

    //insert new solutions
    uint size_soln = (rand() % 20);
    UniqueVector<cnst_lit_sptr,LiteralHasher,LiteralHasher> inserted;
    typedef UniqueVector<cnst_lit_sptr,LiteralHasher,LiteralHasher> list;
    auto insertSoln = [&](list* answers=nullptr){
        if( answers ){
            for (auto &&i : *answers)
            {
                Substitution* variant = getRandVariant(i.get());
                assert_false(ansList.addAnswer(i, *variant));
                delete variant;
            }
            return;
        }
        for (size_t i = 0; i < size_soln; i++)
        {
            ushort depth = (rand() % 2) +1;
            ushort max_arity = (rand() % 10) + 10;
            auto l = getRandLiteral(depth,max_arity).first;
            if( ansList.addAnswer(l, Substitution())){
                inserted.push_back(l);
            }
        }
    };
    insertSoln();
    auto [ssize, newsize] = ansList.size();
    assert_true( (ssize == 0 and newsize == inserted.size() ));

    //assert variant solutions aren't inserted
    insertSoln(&inserted);
    auto [samesize, sameNewsize] = ansList.size();
    assert_true( (samesize == 0 and sameNewsize == inserted.size() ));

    insertQueries(0);
    //Include the new solutions
    ansList.next();
    ansList.apply([](Query&){});


    auto isize = ansList.size();
    assert_true( (isize.first == inserted.size() and isize.second == 0 ));

    //Make sure each query comsumes inserted.size() amounts solutions.
    insertQueries(inserted.size());
    ansList.next(); //Just to collect queries that have consumed all the previous solns
    ansList.apply(accummulate);

    //clear previous solns
    inserted.clear();

    //add new solutions
    size_soln = (rand() % 20);
    insertSoln();

    auto size2nd = ansList.size();
    assert_true( (size2nd.first == isize.first and size2nd.second == inserted.size() ));
    
    //Include the new solutions
    ansList.next();
    auto size2ndNxt = ansList.size();
    assert_true( ((size2ndNxt.first == isize.first + inserted.size()) and size2ndNxt.second == 0 ));


    //Assert that only the new solutions are consumed
    assert_true(queries.size() >= 5);
    for (auto &&q : queries)
    {
        auto it = ansList.addObserver(std::move(q));
        assert_true( (it.end() - it.begin() )== inserted.size() );
        //Assert that every new soln is actually iterated over
        for (auto &&soln : it)
            assert_true( std::find( inserted.begin(), inserted.end(), soln) != inserted.end() );
        
    }
    
}