#ifndef GAME_ANALYZER_HH
#define GAME_ANALYZER_HH
#include "reasoner/reasoner.hh"
namespace ares
{
    class GameAnalyzer
    {
     public:
        GameAnalyzer(Reasoner& reasoner_);

        /**
         * Is the game an alternating zero-sum game.
         * @returns the order of the roles based on their turns.i.e
         * (1st,2nd,...) player
         */
        UniqueVector<ushort> isAZSG(const Game* game, uint startClk);
        bool isSinglePlayer(const Game* game)
        {
            return game->roles().size() == 1;
        }
        uint numPlayers(const Game* game) { return game->roles().size(); }
        void stop()
        {
            std::unique_lock<std::mutex> lk(lock);
            done = true;
            cv.notify_all();
        }
        ~GameAnalyzer()
        {
            if (pool) {
                ThreadPoolFactroy::deallocate(pool);
                ThreadPoolFactroy::deallocate(poolZero);
            }
        }

     private:
        typedef std::unique_ptr<const State, std::function<void(const State*)>>
            uState;
        typedef std::function<void(uState)> Fn;

        void checkAlt(uState, UniqueVector<ushort>&, std::atomic_bool&);
        void checkZero(const State*, std::atomic_bool&);

     private:
        Reasoner& reasoner;
        ThreadPool* pool;
        ThreadPool* poolZero;
        std::atomic_bool done;
        std::mutex lock;
        std::condition_variable cv;
    };
}  // namespace ares

#endif