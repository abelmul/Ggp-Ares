#ifndef REASONER_HH
#define REASONER_HH
#include "utils/gdl/gdl.hh"
#include "utils/game/match.hh"
#include "prover.hh"

namespace ares
{
    typedef std::shared_ptr<CallBack> SharedCB;
    class Reasoner
    {
    public:
        Reasoner(Game& _g, GdlParser& _p, Prover& _prover,MemCache& mem)
        :game(_g)
        ,parser(_p)
        ,prover(_prover)
        ,memCache(mem)
        ,ROLE_GOAL(makeGoal(_p,ROLE_QUERY))
        ,INIT_GOAL(makeGoal(_p,INIT_QUERY))
        ,LEGAL_GOAL(makeGoal(_p,LEGAL_QUERY))
        ,NEXT_GOAL(makeGoal(_p,NEXT_QUERY))
        ,TERMINAL_GOAL( makeGoal(_p,TERMINAL_QUERY) )
        ,GOAL_GOAL( makeGoal(_p,GOAL_QUERY))
        ,TRUE_LITERAL(_p.parseQuery(TRUE_QUERY))
        ,x(memCache.getVar(Namer::X))
        ,r(memCache.getVar(Namer::R))
        {
            goals = std::vector<const Clause*>{ROLE_GOAL,INIT_GOAL,LEGAL_GOAL,NEXT_GOAL,TERMINAL_GOAL,GOAL_GOAL};
            initRoles();        //might as well just get the roles now
            _init();            //similar with the initial state
           initMapping();      //Role to legal/goal query mapping
        }

        static Clause* makeGoal(GdlParser& _p, const char * q){
            return new Clause(nullptr, new ClauseBody{_p.parseQuery(q)});
        }
        /**
         * @returns all the roles in the game
         */
        const Roles& getRoles(){return roles;}
        /**
         * @returns the initial state
         */
        const State& getInit(){return init;}
        /**
         * @returns the next state following from @param state if the players
         * made move @param moves, the moves of each role should have the same 
         * order as the roles in roles. moves[i] is the action taken by role[i].
         */
        State* getNext(const State& state,Moves& moves);
        /**
         * What are the legal moves in the state @param state
         */
        Moves* legalMoves(const State& state, Role& role);

        /**
         * Is @param state a terminal state?
         */
        bool isTerminal(const State& state);

        /**
         * @returns the reward associated with this role in the @param state.
         */
        float getReward(Role& role, const State* state);
        
        ~Reasoner(){
            for (auto &&c : goals)
                delete c;
        }
    
    // private:

        /**
         * Infers the roles within the game
         */
        void initRoles();
        /**
         * Infers the initial state of the game
         */
        void _init();

        /**
         * Just a wrapper method.
         */
        void query(const Clause* goal,const State* context,SharedCB cb);

        /**
         * For ease of access to legal and goal queries of the form (legal some_role ?x)/(goal some_role ?x).
         * initializes roleLegalMap and roleGoalMap.
         */
        void initMapping();

        
        Game& game;
        GdlParser& parser;
        Prover& prover;
        MemCache& memCache;

        
        //These dont change throughout the game.
        Roles roles;
        State init;

        //Just to "pre-create" and hold the legal query, (legal some_role ?x)
        std::unordered_map<ushort, cnst_lit_sptr> roleLegalMap;
        //Just to "pre-create" and hold the goal query, (goal some_role ?x)
        std::unordered_map<ushort, cnst_lit_sptr> roleGoalMap;

        const Clause* ROLE_GOAL;
        const Clause* INIT_GOAL;
        const Clause* LEGAL_GOAL;
        const Clause* NEXT_GOAL;
        const Clause* TERMINAL_GOAL;
        const Clause* GOAL_GOAL;
        cnst_lit_sptr TRUE_LITERAL;
        //Define the queries these don't change through out the lifetime of the player
        std::vector<const Clause* > goals;

        cnst_var_sptr x;   
        cnst_var_sptr r;
        static const char* ROLE_QUERY    ;
        static const char* INIT_QUERY    ;
        static const char* LEGAL_QUERY   ;
        static const char* NEXT_QUERY    ;
        static const char* GOAL_QUERY    ;
        static const char* TRUE_QUERY    ;
        static const char* TERMINAL_QUERY;

        friend struct NxtCallBack;
        friend struct LegalCallBack;
        friend struct RewardCallBack;
        
    };

    /**
     * Just a bunch of callbacks to store computed answer.
     */
    struct NxtCallBack : public CallBack
    {
        NxtCallBack(Reasoner* _t, State* s):CallBack(_done, nullptr),_this(_t),newState(s),_done(false){}

        virtual void operator()(const Substitution& ans,ushort,bool){
            // isCurrent()
            VarSet vset;
            const cnst_term_sptr& true_ = (*_this->TRUE_LITERAL)(ans,vset);      //Instantiate
            if(true_)
                newState->add(Namer::TRUE, new Clause(*((cnst_lit_sptr*)&true_), new ClauseBody(0) ));      //This is thread safe
        }
        ~NxtCallBack(){
            _this = (Reasoner*)0xbeef;
            newState = (State*)0xbeef;
        }
        Reasoner* _this;
        State* newState;
        std::atomic_bool _done;
    };
    struct LegalCallBack : public CallBack
    {
        LegalCallBack(Reasoner* _t):CallBack(_done, nullptr),_this(_t),moves(new Moves()),_done(false){}
        virtual void operator()(const Substitution& ans,ushort,bool){
            // isCurrent()
            VarSet vset;
            const move_sptr& move = (*_this->x)(ans,vset);
            if( move ){
                std::lock_guard<SpinLock> lk(slk);
                moves->push_back(move);
            }
        }
        Reasoner* _this;
        Moves* moves;
        SpinLock slk;
        std::atomic_bool _done;
    };
    struct TerminalCallBack : public CallBack
    {
        TerminalCallBack():CallBack(_done, nullptr),_done(false){}
        virtual void operator()(const Substitution& ans,ushort,bool){
            done = true;
            terminal = true;
        }
        bool terminal = false;
        std::atomic_bool _done;
    };
    struct RewardCallBack : public CallBack
    {
        RewardCallBack(Reasoner* t):CallBack(_done, nullptr),_this(t),_done(false){}
        virtual void operator()(const Substitution& ans,ushort,bool){
            done = true;
            VarSet vset;
            const cnst_term_sptr& rewardTerm =(* _this->x)(ans, vset);
            reward = atof(Namer::name(rewardTerm->get_name()).c_str());
            std::cout << "Reward is : " << reward << "\n";
        }
        float reward;
        Reasoner* _this;
        std::atomic_bool _done;
    };

} // namespace Ares

#endif