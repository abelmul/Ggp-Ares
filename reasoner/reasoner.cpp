#include <reasoner/reasoner.hh>
namespace ares
{
    const char* Reasoner::ROLE_QUERY        = "(role ?r)";
    const char* Reasoner::INIT_QUERY        = "(init ?x) ";
    const char* Reasoner::LEGAL_QUERY       = "(legal ?r ?x)";
    const char* Reasoner::NEXT_QUERY        = "(next ?x)";
    const char* Reasoner::GOAL_QUERY        = "(goal ?r ?x)";
    const char* Reasoner::TRUE_QUERY        = "(true ?x)";
    const char* Reasoner::TERMINAL_QUERY    = "terminal";
    

    typedef std::shared_ptr<CallBack> SharedCB;
    void Reasoner::query(const Clause* goal,const State* context,SharedCB cb){
        auto g = std::unique_ptr<Clause>(goal->clone());
        g->setSubstitution(new Substitution());
        Query query(g, cb,context,new Cache(),0);
        prover->compute(query);
    }
    
    State* Reasoner::getNext(const State& state,const Moves& moves){
        auto& roles = game->getRoles();
        State* context = new State();
        std::vector<Clause*> does;
        (*context) += state;
        PoolKey key;
        for (size_t i = 0; i < moves.size(); i++)
        {
            Body* body = new Body{roles[i], moves[i]};
            key = PoolKey{Namer::DOES, body,true,nullptr};
            auto l = memCache.getLiteral(key);
            does.push_back(new Clause(l,new ClauseBody(0)));
            context->add(Namer::DOES, does.back());           //This is thread safe
        }
        auto cb = std::shared_ptr<NxtCallBack>(new NxtCallBack(this, new State()));
        query(NEXT_GOAL, context, cb);

        for (auto &&d : does)
            delete d;
        
        return cb->newState;
    }

    Moves* Reasoner::legalMoves(const State& state,const Role& role){
        const cnst_lit_sptr& legal = roleLegalMap[role.get_name()];        //Get the legal query specific to this role 
        auto cb = std::shared_ptr<LegalCallBack>(new LegalCallBack(this));
        query(new Clause(nullptr, new ClauseBody{legal}), &state, cb);
        return cb->moves;
    }
    
    bool Reasoner::isTerminal(const State& state){
        auto cb = std::shared_ptr<TerminalCallBack>(new TerminalCallBack());
        query(TERMINAL_GOAL, &state,cb);
        return cb->terminal;
    }
    
    float Reasoner::getReward(Role& role, const State* state){
        cnst_lit_sptr& goal = roleGoalMap[role.get_name()];        //Get the query specific to this role
        auto cb = std::shared_ptr<RewardCallBack>(new RewardCallBack(this));
        query(new Clause(nullptr, new ClauseBody{goal}), state, cb);
        return cb->reward;
    }

    void Reasoner::initMapping(){
        auto& roles = game->getRoles();
        if( roleLegalMap.size() > 0) return;
        /**
         * Template for (legal some_role ?x) and (goal some_role ?x)
         */
        auto& template_body_legal = LEGAL_GOAL->front()->getBody();
        auto& template_body_goal = GOAL_GOAL->front()->getBody();

        PoolKey key_legal{Namer::LEGAL, nullptr, true,nullptr};
        PoolKey key_goal{Namer::GOAL, nullptr, true,nullptr};


        for (auto &&r : roles)
        {
            //Init legal query for role
            key_legal.body = new Body(template_body_legal.begin(), template_body_legal.end());
            (*(Body*)key_legal.body)[0] = r;
            const cnst_lit_sptr& legal_l = memCache.getLiteral(key_legal);
            roleLegalMap[r->get_name()] = legal_l;
            
            //Init goal query for role
            key_goal.body = new Body(template_body_goal.begin(), template_body_goal.end());;
            (*(Body*)key_goal.body)[0] = r;
            const cnst_lit_sptr& goal_l = memCache.getLiteral(key_goal);
            roleGoalMap[r->get_name()] = goal_l;
        }
    }

} // namespace ares
