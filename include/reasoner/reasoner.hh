#ifndef REASONER_HH
#define REASONER_HH
#include "utils/gdl/gdl.hh"
#include "utils/game/match.hh"
#include "prover.hh"

namespace Ares
{
    typedef std::vector<Constant*> Roles;
    typedef std::vector<Term*> Moves;
    template<class T>
    class Reasoner
    {
    public:
        Reasoner(Game* _g):game(_g){}
        /**
         * @returns all the roles in the game
         */
        Roles getRoles();
        /**
         * @returns the initial state
         */
        State* getInit();
        /**
         * @returns the next state following from @param state if the players
         * made move @param moves, the moves of each role should have the same 
         * order as the roles in roles. moves[i] is the action taken by role[i].
         */
        State* getNext(State& state,Moves& moves);
        /**
         * What are the legal moves in the state @param state
         */
        Moves legalMoves(State& state);

        /**
         * Is @param state a terminal state?
         */
        bool isTerminal(State& state);

        /**
         * @returns the reward associated with this role in the @param state.
         */
        float getReward(Constant& role, State* state);

        ~Reasoner(){
            for (auto &&c : queries)
                delete c;
        }
    
    private:
        Game* game;


        //Define the queries these don't change throught out the lifetime of the player
        const std::vector<const Query<T>* > queries{ROLE_QUERY,INIT_QUERY,LEGAL_MOVES_QUERY,NEXT_QUERY,TERMINAL_QUERY,REWARD_QUERY};
        static const Query<T>* ROLE_QUERY;
        static const Query<T>* INIT_QUERY;
        static const Query<T>* LEGAL_MOVES_QUERY;
        static const Query<T>* NEXT_QUERY;
        static const Query<T>* TERMINAL_QUERY;
        static const Query<T>* REWARD_QUERY;
    };
    
} // namespace Ares

#endif