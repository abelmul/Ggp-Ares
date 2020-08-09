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
    
    void Reasoner::query(const Clause* goal,const State* context,const CallBack& cb, bool one){
        bool done = false;
        auto* g = goal->clone();
        g->setSubstitution(new Substitution());
        Query query(std::unique_ptr<Clause>(g),context, std::ref(cb), one, std::ref(done));
        prover.prove(query);
    }
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

        query(ROLE_GOAL, nullptr,std::ref(cb) , false);
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
            const auto& true_ = memCache.getLiteral(key);
            init.add(Namer::TRUE, new Clause(true_, new ClauseBody(0) ));      //This is thread safe
        };

        query(INIT_GOAL,nullptr, std::ref(cb), false);
    }
    
    State* Reasoner::getNext(const State& state,Moves& moves){
        if( roles.size() == 0 ) 
            throw std::runtime_error("Reasoner : Error : Reasoner::getNext called before Reasoner::getRoles!");
        
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
        NxtCallBack cb = NxtCallBack(this, new State());
        query(NEXT_GOAL, context, std::ref(cb), false);

        for (auto &&d : does)
            delete d;
        
        return cb.newState;
    }

    Moves* Reasoner::legalMoves(const State& state,Role& role){
        const cnst_lit_sptr& legal = roleLegalMap[role.get_name()];        //Get the legal query specific to this role 
        LegalCallBack cb(this);
        query(new Clause(nullptr, new ClauseBody{legal}), &state, std::ref(cb), false);
        return cb.moves;
    }
    
    bool Reasoner::isTerminal(const State& state){
        TerminalCallBack cb;
        query(TERMINAL_GOAL, &state, std::ref(cb), true);
        return cb.terminal;
    }
    
    float Reasoner::getReward(Role& role, const State* state){
        cnst_lit_sptr& goal = roleGoalMap[role.get_name()];        //Get the query specific to this role
        RewardCallBack cb(this);
        query(new Clause(nullptr, new ClauseBody{goal}), state, std::ref(cb), true);
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
