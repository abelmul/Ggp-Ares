#include "common.hh"
#include "reasoner/cache.hh"

#include <unordered_set>

using namespace ares;
void setup();
void Hashing_Correct();
void Hashing_Incorrect();
void AnswerIterator();
void Hashing_Variants();
void CacheTest(Cache& cache);
void SeqCacheTest();
void RandCacheTest();

namespace ares
{
    std::atomic<int> Query::nextId = 0;
    AnsIterator Cache::NOT_CACHED(nullptr, -1, nullptr);
}  // namespace ares
int main()
{
    setup();
    Runner runner;
    runner.iter = 100;
    add_test(runner, Hashing_Correct);
    add_test(runner, Hashing_Incorrect);
    add_test(runner, Hashing_Variants);
    add_test(runner, SeqCacheTest);
    runner();
    return 0;
}

/**
 * Variant atoms should hash the same and also be equal.
 * let A1 be an atom and θ := {x0/0, ..., xn/n} where xi is the ith distict
 * variable to occur when counting from the left. The AtomHasher::hash(A1) ==
 * A1(θ).hash() and also A1.equals( A1(θ) )
 */
void Hashing_Correct()
{
    ushort depth = (rand() % 2) + 1;
    ushort max_arity = (rand() % 10) + 10;

    // Get a random atom, and the variables that occur within it ordered by
    // place of first occurence.
    auto [atom, vars] = getRandAtom(depth, max_arity, 3);

    // Create a renaming, θ := {x0/0, ..., xn/n}
    VariantSubstitution theta;
    VarRenaming renaming;

    uint i = 0;
    for (auto&& v : vars) theta.bind(v, Ares::memCache->getVar(i++));

    VarSet vset;
    const auto renamed = (const Atom*)(*atom)(theta, vset);

    /**
     * Positive tests, against the base renaming θ := {x0/0, ..., xn/n}
     */
    // Compare the hashes
    assert_true((AtomHasher()(atom) == renamed->hash()));
    assert_true((AtomHasher()(renamed) == AtomHasher()(atom)));
    // Should be equal
    assert_true((atom->equals(*renamed, renaming)));
    renaming.clear();
    assert_true((atom->equals(*atom, renaming)));
    renaming.clear();
    assert_true((renamed->equals(*atom, renaming)));
    renaming.clear();
    assert_true((renamed->equals(*renamed, renaming)));
}
/**
 * Variant atoms should hash the same and also be equal.
 * let A1 be an atom and θ := {x0/0, ..., xn/n} where xi is the ith distict
 * variable to occur when counting from the left. The AtomHasher::hash(A1) ==
 * A1(θ).hash() and also A1.equals( A1(θ) )
 */
void Hashing_Incorrect()
{
    /**
     * Negative tests
     */
    ushort depth = (rand() % 2) + 1;
    ushort max_arity = (rand() % 10) + 10;

    VariantSubstitution sigma;
    uint i = 0;
    const Atom* atom;
    OrdrdVarSet vars;
    do {
        auto [l, v] = getRandAtom(depth, max_arity, 3);
        atom = l;
        vars = v;
    } while (atom->is_ground());

    VarRenaming renaming;

    uint change;
    if (vars.size() > 0)
        change = rand() % vars.size();

    for (auto&& v : vars) {
        if (i == change) {
            sigma.bind(v, getRandConst(vars, depth));
            continue;
        }
        sigma.bind(v, Ares::memCache->getVar(i++));
    }
    VarSet vset;
    const auto* renamed = (const Atom*)(*atom)(sigma, vset);

    // Compare the hashes
    renaming.clear();
    assert_false(((AtomHasher()(atom) == renamed->hash()) and
                  atom->equals(*renamed, renaming)));
    renaming.clear();
    assert_false(((AtomHasher()(renamed) == AtomHasher()(atom)) and
                  atom->equals(*renamed, renaming)));
    // Should not be equal
    renaming.clear();
    assert_false((atom->equals(*renamed, renaming)));
    renaming.clear();
    assert_false((renamed->equals(*atom, renaming)));
}
/**
 * Variant atoms should hash the same and also be equal.
 * let A1 be an atom and θ := {x0/y1, ..., xn/yn} then
 * AtomHasher::hash(A1) == A1(θ).hash() and also
 * A1.equals( A1(θ) )
 */
void Hashing_Variants()
{
    ushort depth = (rand() % 2) + 1;
    ushort max_arity = (rand() % 10) + 10;

    VariantSubstitution sigma;
    auto [atom, vars] = getRandAtom(depth, max_arity, 3);
    VarRenaming renaming;
    // Create a random renaming
    OrdrdVarSet varsnew;
    std::unordered_set<const Variable*> seen;
    for (auto&& v : vars) {
        auto* vp = getRandVar(varsnew, depth);
        while (seen.find((Variable*)vp) != seen.end()) {
            vp = getRandVar(varsnew, depth);
        }
        sigma.bind(v, vp);
        seen.insert((Variable*)vp);
    }

    VarSet vset;
    auto* renamed = (const Atom*)(*atom)(sigma, vset);
    // Compare the hashes
    assert_true((AtomHasher()(atom) == AtomHasher()(renamed)));
    assert_true((AtomHasher()(renamed) == AtomHasher()(atom)));
    // Should be equal
    assert_true((atom->equals(*renamed, renaming)));
    renaming.clear();
    assert_true((renamed->equals(*atom, renaming)));
}

void SeqCacheTest()
{
    Cache cache(AnsIterator::SEQ);
    CacheTest(cache);
}

void RandCacheTest()
{
    Cache cache(AnsIterator::RAND);
    CacheTest(cache);
}

void CacheTest(Cache& cache)
{
    std::vector<uint> queries;
    std::vector<Query> newQueries;
    Query::resetid();
    uint qcount = 0;
    auto assertQadded = [&](const Query& q) {
        qcount++;
        assert(std::find(queries.begin(), queries.end(), q.id) !=
               queries.end());
    };
    // Create a solution node
    auto [atom, vars] = getRandAtom(3, 15);
    while (atom->is_ground()) {
        auto [l, v] = getRandAtom(3, 15);
        atom = l;
        vars = v;
    }
    auto clause = getRandClause();
    while (clause->size() == 0) clause = getRandClause();

    std::atomic_bool done;
    std::shared_ptr<CallBack> cb(new ClauseCBOne(done, nullptr));
    Query q(clause, cb, nullptr, 0, 0, false);
    q->front() = atom;

    assert_true(cache[q].null());

    // insert solutions
    uint nSoln = (rand() % 10) + 1;
    std::unordered_set<ushort> seenConst;
    OrdrdVarSet dummy;
    ushort d;

    auto addAns = [&, atom(atom), vars(vars)]() {
        for (size_t i = 0; i < nSoln; i++) {
            // just bind the first variable to
            bool added = false;
            do {
                auto c = getRandConst(dummy, d);
                while (seenConst.find(c->get_name()) != seenConst.end())
                    c = getRandConst(dummy, d);
                Substitution theta;
                theta.bind(*vars.begin(), c);
                added = cache.addAns(atom, theta);
            } while (!added);
            assert_true(cache.hasChanged());
        }
    };
    auto addQueries = [&, atom(atom)](const uint&& soln) {
        uint nQ = (rand() % 10) + 3;
        for (size_t i = 0; i < nQ; i++) {
            auto uC2 = getRandClause(1);
            while (uC2->size() == 0) uC2 = getRandClause();

            Query q2(uC2, cb, nullptr, 0, 0, false);
            queries.push_back(q2.id);
            q2->front() = atom;
            auto it = cache[q2];
            newQueries.push_back(q2);
            assert_false(it.null());
            assert_true(it.remaining() == soln);
        }
    };
    // there are no solutions.
    addQueries(0);
    addAns();
    // there are no prev solutions, new solutions should not have taken affect
    // now.
    addQueries(0);

    cache.next(assertQadded);
    assert_true(qcount == queries.size());
    qcount = 0;

    // newQueries
    uint nQ = (rand() % newQueries.size());
    auto restart = [&, atom(atom)](const uint& solnExpected) {
        queries.clear();
        for (size_t i = 0; i < nQ; i++) {
            newQueries[i].goal = getRandClause();
            while (newQueries[i]->size() == 0)
                newQueries[i].goal = getRandClause();

            newQueries[i]->front() = atom;
            auto it = cache[newQueries[i]];
            queries.push_back(newQueries[i].id);

            assert_false(it.null());
            // Should be able to consume the previous solutions.
            assert_true(it.remaining() == solnExpected);
        }
    };

    // Consume all the answers
    restart(nSoln);
    // Assert no answerlist is queued
    auto assertNoSoln = [](const Query&) { assert_true(false); };
    cache.next(assertNoSoln);
    assert_false(cache.hasChanged());
    // No answer should be consumed
    restart(0);
    assert_false(cache.hasChanged());
    // Add new answers
    nSoln = (rand() % 10) + 1;
    addAns();
    // for  (size_t i = 0; i < nQ; i++) {}
    cache.next(assertQadded);

    restart(nSoln);

    // assert_true( )
    // Create a lookup node
    // use the solutions
    // insert new solutions
    // use this new solutions
}