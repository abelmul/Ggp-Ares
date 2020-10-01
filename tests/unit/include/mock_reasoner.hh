#ifndef MOCK_REASONER
#define MOCK_REASONER
#include "reasoner/reasoner.hh"
#include "strategy/montecarlo.hh"
#include "strategy/montecarlo_seq.hh"
namespace ares
{
    class MockReasoner : public Reasoner
    {
    public:
        MockReasoner(GdlParser& p, Prover& prover_,MemCache& mem)
        :Reasoner(p,prover_,mem)
        {initGameTree();}
        void initGameTree();

        /**
         * @returns the initial state
         */
        virtual const State& init();
        /**
         * infer the roles
         */
        virtual const Roles& roles();
        /**
         * @param state the current state
         * @param action an ordered (by role order) list of moves taken by roles. 
         *  i.e action[i] is the move taken by role[i].
         * @returns the next state following from state by taking action.
         */
        virtual State* next(const State& state,const Action& action);
        /**
         * What are the legal moves in the state @param state
         */
        virtual Moves* moves(const State& state,const Role& role,bool rand);
        /**
         * Is @param state a terminal state?
         */
        virtual bool terminal(const State& state);

        /**
         * @returns the reward associated with this role in the @param state.
         */
        virtual float reward(Role& role, const State* state);
        

        /**
         * Some helper functions.
         */
        
        virtual  ushort roleIndex(ushort name)const;
            /**
         * Get a random move.
         */
        virtual Move* randMove(const State& state,const Role& role);
        /**
         * Get a random action, i.e <move_1,...,move_n> , where move_i is taken by role i.
         */
        virtual Action* randAction(const State& state);

        
        /**
         * Get all possible actions from this state,
         * an action = <move_1,...,move_n> , where move_i is taken by role i.
         */
        virtual std::vector<uAction>* actions(const State& state);
        
        virtual void reset(Game* kb);
        ~MockReasoner() {}

    private:
        class Node{
        public:
            typedef std::unique_ptr<Node> uNode;
            typedef std::unordered_map<Action*, uNode> Transition;

            Node(State* s):value(0),self(s){}

            Node(const Node&) = delete;
            Node& operator=(const Node&) = delete;
            Node(const Node&&) = delete;
            Node& operator=(const Node&&) = delete;

            void add(Action* a, Node* n){
                original[a] = a;
                delta[a].reset(n);
            }
            void addEqui(Action* a, Action* a2){
                original[a] = a2;
            }
            Node& operator[](const Action* a){
                auto* key = original.at(a);
                return *delta.at(key);;
            }
            const Action* getOrig(const Action* a) const{
                return original.at(a);
            }
            const Transition& get(){
                return delta;
            }
            bool terminal(){
                return delta.size() ==0;
            }
            State* getState(){ 
                return self;
            }
            
            float value;
        private:
                Transition delta;
                std::unordered_map<const Action*,Action*> original;
                State* self;
        friend class MonteTester;
        };
        class GameTree{
        public:
            typedef std::unique_ptr<Node> uNode;
            typedef std::unordered_map<Action*, uNode> Transition;

            GameTree(){
                max_branching = (rand() % 5) + 3;
                max_depth = (rand() % 5) + 3;
            }
            State* init(){
                return root->getState();
            }

            void addNode(Node* n){
                if( not root ) root.reset(n);
                original[n->getState()] = n->getState();
                nodes[n->getState()] = n;
            }
            void addEqui(State* s, State* s2){
                original[s] = s2;
            }
            Node& operator[](const State* s){
                auto* key = original.at(s);
                return *nodes.at(key);
            }


            uint max_branching;
            uint max_depth;
        private:
            std::unordered_map<const State*,State*> original;
            std::unordered_map<const State*, Node*> nodes;
            uNode root;
        friend class MonteTester;
        };

        GameTree game;
        friend class MonteTester;
    };

    class Montecarlo;
    /**
     * The tester
     */
    class MonteTester
    {
    private:
        Montecarlo& monte;
        MockReasoner& reasoner;
        std::random_device rd;
        std::mt19937 e2;
    public:
        MonteTester(MockReasoner& reasoner_);
        std::vector<Montecarlo::Node*> TestExpansion(MockReasoner::GameTree& game,Montecarlo::Node* node,MockReasoner::Node& mcRNode);
        void TestSelection();
        void TestSim();
        void TestProcess();
        ~MonteTester() {}
    };
} // namespace ares

#endif