#include "runner.hh"
#include "static.hh"

void testBasic();
void testOr();
void testNot();
void testNested();
void testFile();
using namespace std;
using namespace ares;
ares::Cfg ares::cfg;
GdlParser& parser(GdlParser::create(mempool.getCache()));
int main()
{
    Ares::setMem(&mempool);
    // Setup some static elements
    ClauseCB::prover = &Prover::create();
    Body::mempool = &mempool;

    // Create Ares
    Reasoner::create(GdlParser::create(mempool.getCache()), Prover::create(),
                     *mempool.getCache());
    Runner runner;
    runner.iter = 1;
    add_test(runner, testFile);
    add_test(runner, testBasic);
    add_test(runner, testOr);
    add_test(runner, testNot);
    add_test(runner, testNested);
    runner();

    return 0;
}
void assert_equal(vector<string>& tokens, const Clause* c)
{
    tokens.pop_back();
    tokens.erase(tokens.begin());
    string first;
    for (auto&& t : tokens) first += t;

    string second = c->to_string();
    boost::regex re(R"(\s+)");
    second = boost::regex_replace(second, re, "");
    assert_true(first == second);
}
void assert_equal(const char* g, const Clause* c)
{
    vector<string> tokens;
    parser.tokenize(g, tokens);
    assert_equal(tokens, c);
}
void assert_equal(string s1, string s2)
{
    boost::regex re(R"(\s+)");
    s1 = boost::regex_replace(s1, re, "");
    s2 = boost::regex_replace(s2, re, "");
    assert_true(s1 == s2);
}
void testFile()
{
    Game* game = new Game();
    std::string gdl("tests/ggp.org/games/pentago.kif");
    parser.parse(game, gdl);
}
void testBasic()
{
    vector<string> fact;
    vector<string> rule;
    parser.tokenize("(( init (cell ?x 3 blank) ))", fact);
    parser.tokenize(
        "((<= (next (cell ?x ?y ?p)) (does ?player (move ?piece ?x1 ?y1 ?x2 "
        "?y2)) ))",
        rule);
    Game* game = new Game();

    parser.parse(game, fact);
    const auto& rules = game->getRules();
    assert_true(rules.size() == 1);

    auto asserter =
        [&](const std::unordered_map<ushort, ares::UniqueClauseVec*>& rules_,
            std::vector<std::string>& tokens, ushort name) {
            auto it = rules_.find(name);
            assert_true(it != rules_.end());
            assert_true(it->second->size() == 1);
            assert_equal(tokens, (*it->second)[0]);
        };

    Game* game2 = new Game();
    parser.parse(game2, rule);
    const auto& rulesR = game2->getRules();
    assert_true(rulesR.size() == 1);

    asserter(rules, fact, Namer::INIT);
    asserter(rulesR, rule, Namer::NEXT);
    delete game;
    delete game2;
}

void testOr()
{
    vector<string> rule;
    parser.tokenize(
        "( (<= (legal ?player (move ?piece ?u ?v ?x ?y))\
    (true (control ?player))\
    (or (true (check ?player rook ?tx ?ty))\
        (true (check ?player queen ?tx ?ty)))) )",
        rule);

    Game* game = new Game();
    parser.parse(game, rule);
    const auto* ucv = (*game)[Namer::LEGAL];
    assert_true(ucv) assert_true(ucv->size() == 1);
    const auto* clause = (*ucv)[0];
    assert_equal(clause->getHead()->to_string(),
                 "(legal ?player (move ?piece ?u ?v ?x ?y))");
    assert_equal(clause->getBody()[0]->to_string(), "(true (control ?player))");
    assert_equal(clause->getBody()[1]->to_string(),
                 "(or (true (check ?player rook ?tx ?ty))(true (check ?player "
                 "queen ?tx ?ty)))");
    assert_true(clause->getBody()[1]->get_type() == Term::OR);
}

void testNot()
{
    vector<string> rule;
    parser.tokenize(
        "((<= (legal ?player (move ?piece ?u ?v ?x ?y))\
    (or (true (check ?player bishop ?tx ?ty))\
        (true (check ?player queen ?tx ?ty)))\
    (not (occupied_by_player ?x ?y ?player))) )",
        rule);

    Game* game = new Game();
    parser.parse(game, rule);
    const auto* ucv = (*game)[Namer::LEGAL];
    assert_true(ucv) assert_true(ucv->size() == 1);
    const auto* clause = (*ucv)[0];

    assert_equal(clause->getHead()->to_string(),
                 "(legal ?player (move ?piece ?u ?v ?x ?y))");
    assert_equal(clause->getBody()[0]->to_string(),
                 "(or (true (check ?player bishop ?tx ?ty))(true (check "
                 "?player queen ?tx ?ty)))");
    assert_true(clause->getBody()[0]->get_type() == Term::OR);
    assert_equal(clause->getBody()[1]->to_string(),
                 "(not (occupied_by_player ?x ?y ?player))");
    assert_true(clause->getBody()[1]->get_type() == Term::NOT);
}

void testNested()
{
    vector<string> rule;
    parser.tokenize(
        "((<= (legal ?player (move ?piece ?u ?v ?x ?y))\
    (or (not (true (check ?player bishop ?tx ?ty)))\
        (true (check ?player queen ?tx ?ty)))\
    (not (or  (true (check ?player queen ?tx ?ty)) (occupied_by_player ?x ?y ?player) )) ))",
        rule);
    Game* game = new Game();
    parser.parse(game, rule);
    const auto* ucv = (*game)[Namer::LEGAL];
    assert_true(ucv) assert_true(ucv->size() == 1);
    const auto* clause = (*ucv)[0];

    assert_equal(clause->getHead()->to_string(),
                 "(legal ?player (move ?piece ?u ?v ?x ?y))");
    assert_equal(clause->getBody()[0]->to_string(),
                 "(or (not (true (check ?player bishop ?tx ?ty)))(true (check "
                 "?player queen ?tx ?ty)))");
    assert_true(clause->getBody()[0]->get_type() == Term::OR);
    assert_equal(clause->getBody()[1]->to_string(),
                 "(not (or  (true (check ?player queen ?tx ?ty)) "
                 "(occupied_by_player ?x ?y ?player) ))");
    assert_true(clause->getBody()[1]->get_type() == Term::NOT);
}