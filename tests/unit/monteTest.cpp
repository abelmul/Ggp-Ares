#include "mock_reasoner.hh"
#include "runner.hh"
#include "static.hh"

#include <random>
#include <set>
ares::Cfg ares::cfg;
using namespace ares;
Reasoner* reasoner;

void setup()
{
    srand(time(NULL));
    std::cout.setf(std::ios::unitbuf);

    // Read in the configuration file
    cfg = Cfg("./ares.cfg.json");
    log("[Ares] configurations...\n" + cfg.str());

    // Set the global memory pool
    Ares::setMem(&mempool);

    // Setup some static elements
    ClauseCB::prover = &Prover::create();
    Body::mempool = &mempool;

    // Create Ares with the MockReasoner
    reasoner = new MockReasoner(GdlParser::create(mempool.getCache()),
                                Prover::create(), *mempool.getCache());
    Ares::create(Registrar::get(cfg.strategy.c_str()), *reasoner);
}
void uct_test();
int main()
{
    setup();
    uct_test();
    MonteTester monteTester((MockReasoner&)*reasoner);
    Runner runner;
    add_test(runner, [&] { monteTester.TestSelection(); });
    add_test(runner, [&] { monteTester.TestSim(); });
    runner.iter = 100;
    runner();
    return 0;
}

// Very simple uct test
void uct_test()
{
    using Node = Montecarlo::Node;
    auto& uct = Montecarlo::uct;
    Node* parent = new Node(nullptr, nullptr);
    Node* node = new Node(nullptr, nullptr);

    node->n = (rand() % 9) + 1;
    node->values[0] = rand() % 100;
    node->values[1] = rand() % (100 - (int)node->values[0]);
    node->values[2] = 100 - (node->values[0] + node->values[1]);

    node->parent = parent;
    parent->n = (rand() % node->n) + ((rand() % 5) + 1);
    uint c = rand() % 50;
    uint i = 0;
    for (auto&& value : node->values) {
        float expectedUct =
            (value / node->n) + c * sqrt((2 * log(parent->n)) / node->n);
        assert_true(uct(*node, c, i++, 0) == expectedUct);
    }

    // Test default values
    node->n = 0;
    assert_true(uct(*node, c, 0, 0) == 0);
    assert_true(uct(*node, c, 0, 0) == 0);
    assert_true(uct(*node, c, 0, INFINITY) == INFINITY);
}

namespace ares
{
    MonteTester::MonteTester(MockReasoner& reasoner_)
        : monte((Montecarlo&)Registrar::get("Montecarlo")),
          reasoner(reasoner_),
          e2(rd())
    {
    }
    std::vector<Montecarlo::Node*> MonteTester::TestExpansion(
        MockReasoner::GameTree& game, Montecarlo::Node* node,
        MockReasoner::Node& mcRNode)
    {
        using McRNode = MockReasoner::Node;
        using Node = Montecarlo::Node;

        std::set<McRNode*> origState;
        std::set<McRNode*> selectedStates;
        std::vector<Node*> children(node->children.begin(),
                                    node->children.end());
        for (auto&& [action, child] : mcRNode.get()) {
            bool found = false;
            for (auto&& c : node->children) {
                auto* origA = mcRNode.getOrig(c->action.get());
                if (origA == action) {
                    found = true;
                    break;
                }
            }
            if (not found)
                origState.insert(child.get());
        }
        std::atomic_bool d = false;
        for (auto&& _ : origState) {
            Node* v = (*monte.selPolicy)(node, d, 0, 0);
            McRNode* vMc = &game[v->state.get()];
            // Every state should be visited only once during expansion.
            assert_true(selectedStates.find(vMc) == selectedStates.end());
            selectedStates.insert(vMc);
            children.push_back(v);
        }
        assert_true(origState == selectedStates);
        return children;
    }
    void MonteTester::TestSelection()
    {
        reasoner.game = MockReasoner::GameTree();
        reasoner.initGameTree();
        typedef std::function<void(Montecarlo::Node*,
                                   std::reference_wrapper<MockReasoner::Node>)>
            Fn;
        using McRNode = MockReasoner::Node;
        using Node = Montecarlo::Node;
        auto& game = reasoner.game;
        std::uniform_real_distribution<> dist(0, 100);
        std::atomic_bool d = false;

        auto testBestSelection = [&](Node* parent,
                                     const std::vector<Node*>& children) {
            float c = cfg.uct_c;
            assert_true(parent->children == children);
            assert_false((parent->actions));  // Has been fully expanded
            if (not children.size())
                return;

            parent->n = (rand() % 10) + 1;
            // Biase the uct values
            Node* best = nullptr;
            std::vector<Node*> zeros;
            float max = -INFINITY;
            auto i = rand() % 3;
            for (auto&& child : children) {
                child->values[0] = rand() % 100;
                child->values[1] = rand() % (100 - (int)child->values[0]);
                child->values[2] = 100 - (child->values[0] + child->values[1]);
                child->n = (rand() % 10);

                float uctv =
                    child->n == 0
                        ? INFINITY
                        : (child->values[i] / child->n) +
                              c * sqrt((2 * std::log(parent->n)) / child->n);
                if (max < uctv) {
                    best = child;
                    max = uctv;
                }
            }

            // Assuming Everything in parent is expanded
            Node* selected = (*monte.selPolicy)(parent, d, i, i);
            auto isT = (reasoner.terminal(*best->state));
            assert_true((isT and (selected == best)) or
                        ((!isT) and (selected->parent == best)));
        };

        Fn dfsAssert = [&](Node* node, McRNode& mcNode) {
            const auto& children = TestExpansion(game, node, mcNode);
            testBestSelection(node, children);

            for (auto&& child : children) {
                auto& mcChildNode = game[child->state.get()];
                dfsAssert(child, mcChildNode);
            }
        };

        Node* root = new Node(&reasoner.init(), nullptr);
        MockReasoner::Node& rootNode = game[root->state.get()];
        dfsAssert(root, rootNode);
        root->erase();
        root->state.release();
        delete root;
    }

    void MonteTester::TestSim()
    {
        using McRNode = MockReasoner::Node;
        using Node = Montecarlo::Node;

        std::atomic_bool d = false;

        reasoner.game = MockReasoner::GameTree();
        reasoner.initGameTree();
        auto& game = reasoner.game;
        Node* root = new Node(&reasoner.init(), nullptr);
        McRNode& rootNode = game[root->state.get()];
        auto children = TestExpansion(game, root, rootNode);
        assert_true(children == root->children);
        if (children.size()) {
            uint i = rand() % children.size();
            auto* selected = children[i];
            auto val = (*monte.simPolicy)(selected->state.get(), d);
            auto prev = std::pair<std::vector<float>, uint>(selected->values,
                                                            selected->n);
            auto prevR =
                std::pair<std::vector<float>, uint>(root->values, root->n);
            monte.update(selected, val);
            for (size_t i = 0; i < reasoner.roles().size(); i++) {
                assert_true(fabs(selected->values[i] -
                                 (prev.first[i] + val[i])) < 0.001);
                assert_true(fabs(root->values[i] - (prevR.first[i] + val[i])) <
                            0.001);
            }

            assert_true(selected->n == prev.second + 1);
            assert_true(root->n == prevR.second + 1);
        }
    }
    void MonteTester::TestProcess() {}
}  // namespace ares

/**
 *
 */
