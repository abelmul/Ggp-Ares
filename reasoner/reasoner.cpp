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
    
    void Reasoner::initRoles(){
        //the roles dont change once computed
        if( roles.size() > 0 ) return;
        //Just static things
        static SpinLock rM;
        
        auto cb = [this](const Substitution& ans){
            VarSet vset;
            const role_sptr& role = (*this->r)(ans,vset);             //Instantiate
            if(role){
                std::lock_guard<SpinLock> lk(rM);
                this->roles.push_back(role);          //Initialize the roles vector.
            }
        };

        query(ROLE_GOAL, nullptr,cb , false);
    }
    void Reasoner::_init(){
        //the initial state doesn't change once computed
        static auto cb = [this](const Substitution& ans){
            VarSet vset;
            auto& l = (*Reasoner::INIT_GOAL->front())(ans,vset);
            const cnst_lit_sptr& l_init = *(cnst_lit_sptr*)&l;                //Instantiate
            if( not l_init) return;
            //build the 'true' relation
            PoolKey key{Namer::TRUE, new Body(l_init->getBody().begin(),l_init->getBody().end() ), true,nullptr};
            const auto& true_ = expPool.getLiteral(key);
            init.add(Namer::TRUE, new Clause(true_, new ClauseBody(0) ));      //This is thread safe
        };

        query(INIT_GOAL,nullptr, cb, false);
    }
    
    State* Reasoner::getNext(const State& state,Moves& moves){
        if( roles.size() == 0 ) 
            throw std::runtime_error("Reasoner : Error : Reasoner::getNext called before Reasoner::getRoles!");
        
        State* context = new State();
        (*context) += state;
        PoolKey key;
        for (size_t i = 0; i < moves.size(); i++)
        {
            Body* body = new Body{roles[i], moves[i]};
            key = PoolKey{Namer::DOES, body,true,nullptr};
            auto l = expPool.getLiteral(key);
            context->add(Namer::DOES, new Clause(l,new ClauseBody(0)));           //This is thread safe
        }
        auto* s =new State();
        NxtCallBack cb = NxtCallBack(this,  s);

        query<NxtCallBack>(NEXT_GOAL, context, cb, false);
        return cb.newState;
    }

    Moves* Reasoner::legalMoves(const State& state,Role& role){
        const cnst_lit_sptr& legal = roleLegalMap[role.get_name()];        //Get the legal query specific to this role 
        LegalCallBack cb(this);
        query<LegalCallBack>(new Clause(nullptr, new ClauseBody{legal}), &state, cb, false);
        return cb.moves;
    }
    
    bool Reasoner::isTerminal(const State& state){
        TerminalCallBack cb;
        query<TerminalCallBack>(TERMINAL_GOAL, &state, cb, true);
        return cb.terminal;
    }
    
    float Reasoner::getReward(Role& role, const State* state){
        cnst_lit_sptr& goal = roleGoalMap[role.get_name()];        //Get the query specific to this role
        RewardCallBack cb(this);
        query<RewardCallBack>(new Clause(nullptr, new ClauseBody{goal}), state, cb, true);
        return cb.reward;
    }

    void Reasoner::initMapping(){
            if( roles.size() == 0 )
                throw std::runtime_error("Reasoner : Error : cant init map without roles!");
            if( roleLegalMap.size() > 0) return;
            /**
             * Template for (legal some_role ?x) and (goal some_role ?x)
             */
            auto& template_body_legal = LEGAL_GOAL->front()->getBody();
            auto& template_body_goal = GOAL_GOAL->front()->getBody();


            Body* body_legal = new Body(template_body_legal.begin(), template_body_legal.end());
            Body* body_goal = new Body(template_body_goal.begin(), template_body_goal.end());


            PoolKey key_legal{Namer::LEGAL, body_legal, true,nullptr};
            PoolKey key_goal{Namer::GOAL, body_goal, true,nullptr};


            for (auto &&r : roles)
            {
                //Init legal query for role
                (*body_legal)[0] = r;
                const cnst_lit_sptr& legal_l = expPool.getLiteral(key_legal);
                roleLegalMap[r->get_name()] = legal_l;
                body_legal = new Body(template_body_legal.begin(), template_body_legal.end());
                key_legal.body = body_legal;
                
                //Init goal query for role
                (*body_goal)[0] = r;
                const cnst_lit_sptr& goal_l = expPool.getLiteral(key_goal);
                roleGoalMap[r->get_name()] = goal_l;
                body_goal = new Body(template_body_goal.begin(), template_body_goal.end());
                key_goal.body = body_goal;
            }
            
        }

} // namespace ares
