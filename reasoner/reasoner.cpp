#include <reasoner/reasoner.hh>
namespace ares
{
    const char* Reasoner::ROLE_QUERY = "(role ?r)";
    const char* Reasoner::INIT_QUERY = "(init ?x) ";
    const char* Reasoner::LEGAL_QUERY = "(legal ?r ?x)";
    const char* Reasoner::NEXT_QUERY = "(next ?x)";
    const char* Reasoner::GOAL_QUERY = "(goal ?r ?x)";
    const char* Reasoner::TRUE_QUERY = "(true ?x)";
    const char* Reasoner::TERMINAL_QUERY = "terminal";

    typedef std::shared_ptr<CallBack> SharedCB;
    void Reasoner::query(uClause& goal, const State* context, SharedCB cb,
                         bool rand)
    {
        auto g = std::unique_ptr<Clause>(goal->clone());
        g->setSubstitution(new Substitution());
        auto cache = std::unique_ptr<Cache>(
            new Cache(rand ? AnsIterator::RAND : AnsIterator::SEQ));
        Query query(g, cb, context, cache.get(), 0, rand);
        prover.compute(query);
    }
    void printCacheStat(const Clause* goal, Cache* cache)
    {
        std::cout << "------- Cache Statistics " << goal->to_string()
                  << "------- \n";
        std::cout << " Total Lookups : " << cache->nlookups << "\n";
        std::cout << " Total SolutionNodes : " << cache->nsolns << "\n";
        std::cout << " Total Consumed Answer : " << cache->nconsumedAns << "\n";
        std::cout << " Total New Answers : " << cache->nnewAns << "\n";
        if (cache->nsolns)
            std::cout << " Average Answer generated by a Solution Node : "
                      << (cache->nnewAns / cache->nsolns) << "\n";
        if (cache->nlookups)
            std::cout << " Average Consumed Answer by a Lookup Node : "
                      << (cache->nconsumedAns / cache->nlookups) << "\n";
    }

    const State& Reasoner::init()
    {
        if (game->init())
            return *game->init();

        // compute it
        struct InitCb : public CallBack {
            InitCb(Reasoner* r)
                : CallBack(done_, nullptr),
                  this_(r),
                  init(new State()),
                  done_(false)
            {
            }
            virtual void operator()(const Substitution& ans, ushort, bool)
            {
                VarSet vset;
                auto true_ = (const Atom*)(*this_->TRUE_LITERAL)(
                    ans, vset);  // Instantiate
                if (true_) {
                    auto* cl = new Clause(true_, new ClauseBody(0));
                    if (not init->add(Namer::TRUE, cl))  // This is thread safe
                        delete cl;                       // Duplicate element
                }
            }
            Reasoner* this_;
            State* init;
            std::atomic_bool done_;
        };
        auto cb = new InitCb(this);
        auto scb = std::shared_ptr<CallBack>(cb);
        query(INIT_GOAL, nullptr, scb);
        game->init(cb->init);
        return *game->init();
    }
    const Roles& Reasoner::roles()
    {
        if (game->roles().size())
            return game->roles();

        // compute it
        struct RoleCb : public CallBack {
            RoleCb(Reasoner* r, Game* g)
                : CallBack(done_, nullptr), this_(r), game(g), done_(false)
            {
            }
            virtual void operator()(const Substitution& ans, ushort, bool)
            {
                VarSet vset;
                auto role = (*this_->r)(ans, vset);  // Instantiate
                if (role)
                    game->addRole(role);
            }
            Reasoner* this_;
            Game* game;
            std::atomic_bool done_;
        };
        auto cb = new RoleCb(this, game);
        auto scb = std::shared_ptr<CallBack>(cb);
        query(ROLE_GOAL, nullptr, scb);
        return game->roles();
    }
    State* Reasoner::next(const State& state, const Action& moves)
    {
        auto& roles = game->roles();
        State* context = new State();
        (*context) += state;  // Shallow copy
        PoolKey key;
        for (size_t i = 0; i < moves.size(); i++) {
            Body* body = new Body{roles[i], moves[i]};
            key = PoolKey{Namer::DOES, body};
            auto l = memCache.getAtom(key);
            context->add(
                Namer::DOES,
                new Clause(l, new ClauseBody(0)));  // This is thread safe
        }
        auto cb =
            std::shared_ptr<NxtCallBack>(new NxtCallBack(this, new State()));
        query(NEXT_GOAL, context, cb);

        for (auto&& d : *(*context)[Namer::DOES]) delete d;

        delete (*context)[Namer::DOES];

        context->reset();
        delete context;

        return cb->newState;
    }

    Moves* Reasoner::moves(const State& state, const Role& role, bool rand)
    {
        auto& legal = roleLegalMap[role.get_name()];  // Get the legal query
                                                      // specific to this role
        auto cb = std::shared_ptr<LegalCallBack>(new LegalCallBack(this, rand));
        query(legal, &state, cb, rand);
        return cb->moves;
    }

    bool Reasoner::terminal(const State& state)
    {
        auto cb = std::shared_ptr<TerminalCallBack>(new TerminalCallBack());
        query(TERMINAL_GOAL, &state, cb);
        return cb->terminal;
    }

    float Reasoner::reward(Role& role, const State* state)
    {
        auto& goal = roleGoalMap[role.get_name()];  // Get the query specific to
                                                    // this role
        auto cb = std::shared_ptr<RewardCallBack>(new RewardCallBack(this));
        query(goal, state, cb);
        return cb->reward;
    }

    /**
     *Helper Functions.
     */
    Move* Reasoner::randMove(const State& state, const Role& role)
    {
        auto* legals = moves(state, role);
        uint i = rand() % legals->size();
        auto selected = (*legals)[i];
        delete legals;
        return selected;
    }

    Action* Reasoner::randAction(const State& state)
    {
        Action* action = new Action();
        for (auto&& role : roles()) action->push_back(randMove(state, *role));

        return action;
    }

    std::vector<uAction>* Reasoner::actions(const State& state)
    {
        const auto& roles = this->roles();
        std::vector<uMoves> legals;
        for (auto&& r : roles) legals.push_back(uMoves(moves(state, *r)));

        Moves partials;
        std::vector<uAction>* combos = new std::vector<uAction>();
        getCombos(legals, 0, partials, *combos);
        return combos;
    }
    std::vector<uAction>* Reasoner::actions(std::vector<uMoves>& legals)
    {
        Moves partials;
        std::vector<uAction>* combos = new std::vector<uAction>();
        getCombos(legals, 0, partials, *combos);
        return combos;
    }
    void Reasoner::initMapping()
    {
        auto& roles = game->roles();
        if (roleLegalMap.size() > 0)
            return;

        for (size_t i = 0; i < roles.size(); i++)
            rolesIndex[roles[i]->get_name()] = i;  // from the name to its index

        /**
         * Template for (legal some_role ?x) and (goal some_role ?x)
         */
        auto& template_body_legal = ((Atom*)LEGAL_GOAL->front())->getBody();
        auto& template_body_goal = ((Atom*)GOAL_GOAL->front())->getBody();

        PoolKey key_legal{Namer::LEGAL, nullptr};
        PoolKey key_goal{Namer::GOAL, nullptr};

        for (auto&& r : roles) {
            // Init legal query for role
            key_legal.body = new Body(template_body_legal.begin(),
                                      template_body_legal.end());
            (*(Body*)key_legal.body)[0] = r;
            const Atom* legal_l = memCache.getAtom(key_legal);
            roleLegalMap[r->get_name()].reset(
                new Clause(nullptr, new ClauseBody{legal_l}));

            // Init goal query for role
            key_goal.body =
                new Body(template_body_goal.begin(), template_body_goal.end());
            ;
            (*(Body*)key_goal.body)[0] = r;
            const Atom* goal_l = memCache.getAtom(key_goal);
            roleGoalMap[r->get_name()].reset(
                new Clause(nullptr, new ClauseBody{goal_l}));
        }
    }

}  // namespace ares
