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
    protected:
        /**
         * Ctor
         */
        Reasoner( GdlParser& p, Prover& prover_,MemCache& mem)
        :game(nullptr),parser(p),prover(prover_),memCache(mem)
        {
        }


        Reasoner(const Reasoner&)=delete;
        Reasoner& operator=(const Reasoner&)=delete;
        Reasoner(const Reasoner&&)=delete;
        Reasoner& operator=(const Reasoner&&)=delete;
    
    /**
     * Methods
     */
    public:
        /**
         * The singleton Reasoner;
         */
        static Reasoner& create(GdlParser& p, Prover& prover_,MemCache& mem){
            static Reasoner reasoner(p,prover_,mem);
            return reasoner;
        }

        static Clause* makeGoal(GdlParser& p, const char * q){
            return new Clause(nullptr, new ClauseBody{p.parseQuery(q)});
        }
        /**
         * infer the initial state
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
        virtual Moves* moves(const State& state,const Role& role,bool rand=false);
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
        
        virtual inline ushort roleIndex(ushort name)const{ return rolesIndex.at(name);}
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
        virtual std::vector<uAction>* actions(std::vector<uMoves>&);
        
        virtual inline void reset(Game* kb){
            roleLegalMap.clear();
            roleGoalMap.clear();
            rolesIndex.clear();
            prover.reset(kb);
            if( game ) delete game;
            game = kb;
            if( game ){
                initQueries();
                init();
                roles();
                initMapping();
            }
        }
        virtual inline void initQueries(){
            x = memCache.getVar(Namer::X);
            r = memCache.getVar(Namer::R);
            TRUE_LITERAL = parser.parseQuery(TRUE_QUERY);
            INIT_GOAL.reset( makeGoal(parser,INIT_QUERY));
            LEGAL_GOAL.reset( makeGoal(parser,LEGAL_QUERY));
            NEXT_GOAL.reset( makeGoal(parser,NEXT_QUERY));
            TERMINAL_GOAL.reset(  makeGoal(parser,TERMINAL_QUERY) );
            GOAL_GOAL.reset( makeGoal(parser,GOAL_QUERY));
            ROLE_GOAL.reset( makeGoal(parser,ROLE_QUERY));
        }
        virtual ~Reasoner(){            
            if( game ) delete game;
        }
    
    private:
        typedef std::unique_ptr<const Clause> uClause;
        /**
         * Just a wrapper method.
         */
        void query(uClause& goal,const State* context,SharedCB cb,bool rand=false);

        /**
         * For ease of access to legal and goal queries of the form (legal some_role ?x)/(goal some_role ?x).
         * initializes roleLegalMap and roleGoalMap.
         */
        void initMapping();
        
        /**
         * Get all the possible combination of legals. (Order considered)
         */
        inline void getCombos(std::vector<uAction>& legals, uint i, Moves& partials, std::vector<uAction>& combos){
            if( i >= legals.size()){ combos.push_back( uAction(new Moves(partials.begin(), partials.end()))); return;}

            for (auto &&m : *legals[i])
            {
                partials.push_back(m);
                getCombos(legals,i+1,partials,combos);
                partials.pop_back();
            }
        }

    /**
     * Data
     */
    public:
        const Atom*  TRUE_LITERAL;
        const Variable*  x;   
        const Variable*  r;
    private:
        Game* game;
        GdlParser& parser;
        Prover& prover;
        MemCache& memCache;

        //Just to "pre-create" and hold the legal query, (legal some_role ?x)
        std::unordered_map<ushort, std::unique_ptr<const Clause>> roleLegalMap;
        //Just to "pre-create" and hold the goal query, (goal some_role ?x)
        std::unordered_map<ushort, std::unique_ptr<const Clause>> roleGoalMap;
        //role name to index mapping
        std::unordered_map<ushort, ushort> rolesIndex;

        
        uClause INIT_GOAL;
        uClause LEGAL_GOAL;
        uClause NEXT_GOAL;
        uClause TERMINAL_GOAL;
        uClause GOAL_GOAL;
        uClause ROLE_GOAL;
        
        //Define the queries these don't change through out the lifetime of the 

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
        NxtCallBack(Reasoner* t, State* s):CallBack(done_, nullptr),this_(t),newState(s),done_(false){}

        virtual void operator()(const Substitution& ans,ushort,bool){
            // isCurrent()
            VarSet vset;
            auto true_ = (const Atom*)(*this_->TRUE_LITERAL)(ans,vset);      //Instantiate
            if(true_){
                auto* cl = new Clause(true_, new ClauseBody(0) );
                if ( not newState->add(Namer::TRUE, cl) )//This is thread safe
                    delete cl;  //Duplicate element
            }
        }
        ~NxtCallBack(){
            this_ = (Reasoner*)0xbeef;
            newState = (State*)0xbeef;
        }
        Reasoner* this_;
        State* newState;
        std::atomic_bool done_;
    };
    struct LegalCallBack : public CallBack
    {
        LegalCallBack(Reasoner* t,bool r)
        :CallBack(done_, nullptr),this_(t),moves(new Moves()),done_(false),rand(r),count(0)
        {}
        virtual void operator()(const Substitution& ans,ushort,bool){
            if( rand ){
                ++count;
                if( count >= cfg.ansSample ) done = true;
            }
            VarSet vset;
            const Move* move = (*this_->x)(ans,vset);
            if( move ){
                std::lock_guard<SpinLock> lk(slk);
                moves->push_back(move);
            }
        }
        Reasoner* this_;
        Moves* moves;
        SpinLock slk;
        std::atomic_bool done_;
        bool rand;
        byte count;
    };
    struct TerminalCallBack : public CallBack
    {
        TerminalCallBack():CallBack(done_, nullptr),done_(false){}
        virtual void operator()(const Substitution&,ushort,bool){
            done = true;
            terminal = true;
        }
        bool terminal = false;
        std::atomic_bool done_;
    };
    struct RewardCallBack : public CallBack
    {
        RewardCallBack(Reasoner* t):CallBack(done_, nullptr),reward(0.0),this_(t),done_(false){}
        virtual void operator()(const Substitution& ans,ushort,bool){
            done = true;
            VarSet vset;
            auto* rewardTerm =(* this_->x)(ans, vset);
            reward = atof(Namer::name(rewardTerm->get_name()).c_str());
        }
        float reward;
        Reasoner* this_;
        std::atomic_bool done_;
    };

} // namespace Ares

#endif