#include "gameAnalyzer/gameAnalyzer.hh"

namespace ares
{
    GameAnalyzer::GameAnalyzer(Reasoner& reasoner_)
        : reasoner(reasoner_), done(false)
    {
        static auto nthreads = std::thread::hardware_concurrency() / 2;
        pool = ThreadPoolFactroy::get(nthreads ? nthreads : 1);
        poolZero = ThreadPoolFactroy::get(nthreads ? nthreads : 1);
    }

    UniqueVector<ushort> GameAnalyzer::isAZSG(const Game* game, uint startClk)
    {
        log("[GameAnalyzer]")
            << "Checking Alternating Zero-Sum Property Via Simulations.\n";
        done = false;
        UniqueVector<ushort> order;
        if (game->roles().size() == 1)
            return order;

        const auto* init = game->init();
        auto deleter = [&](const State* s) {
            if (s != init)
                delete s;
        };

        const auto roles = reasoner.roles();
        std::atomic_bool azsg = true;
        std::thread timer([&] {
            auto milli = (startClk * 1000) - cfg.delta_milli;
            {
                std::unique_lock<std::mutex> lk(lock);
                log("[GameAnalyzer]")
                    << "Timer set for " << milli << " milliseconds.\n";
                cv.wait_for(lk, std::chrono::milliseconds(milli),
                            [&] { return (bool)done; });
            }
            done = true;
        });

        pool->post([&] {
            while (not done and azsg) {
                auto us = uState(init, deleter);
                checkAlt(std::move(us), order, azsg);
            }
        });
        poolZero->post([&] { checkZero(init, azsg); });

        pool->wait();
        poolZero->wait();
        log("[GameAnalyzer]") << "Done Checking. Game is ";
        if (not azsg) {
            std::cout << "NOT Alternating Zero-Sum game.\n";
            order.clear();
        } else {
            std::cout << "Alternating Zero-Sum game.\n";
            log("[GameAnalyzer] Order of roles (by turn) is :\n\t\t");
            std::string sep = "";
            std::string str;
            for (size_t i = 0; i < order.size(); i++) {
                str += sep + "Player " + to_string(i + 1) + " : " +
                       reasoner.roles()[order[i]]->to_string();
                sep = ",";
            }
            log(str) << "\n";
        }
        {
            std::lock_guard<std::mutex> lk(lock);
            done = true;
        }
        cv.notify_one();
        timer.join();
        return order;
    }

    void GameAnalyzer::checkAlt(uState state, UniqueVector<ushort>& order,
                                std::atomic_bool& azsg)
    {
        const auto roles = reasoner.roles();
        float ambcount = 0, count = 0;
        /**
         * Check if there is a unique player whose turn it is.
         */
        auto checkUniqueTurn = [&](std::vector<uMoves>& moves) {
            int player = -1;
            bool found = false;
            for (uint r = 0; r < roles.size(); r++) {
                // role r's legal moves
                if (moves[r]->size() > 1 and found) {
                    azsg = false;  // Two players have more than 1 legal move
                    break;
                }

                else if (moves[r]->size() > 1) {
                    found = true;
                    player = r;
                }
            }
            return azsg ? player : -1;
        };

        Fn check = [&](uState state) {
            if (done or not azsg) {
                if (ambcount / count >= 0.09)
                    azsg = false;
                return;
            }
            UniqueVector<ushort> turn;
            bool ambigious = false;
            count++;
            for (auto&& role_ : roles) {
                if (reasoner.terminal(*state) or done)
                    break;
                std::vector<uMoves> moves(roles.size());
                for (uint r = 0; r < roles.size(); r++)
                    moves[r].reset(reasoner.moves(*state, *roles[r]));

                int player = ambigious ? -1 : checkUniqueTurn(moves);
                if (not azsg)
                    break;

                if ((player == -1))
                    ambigious = true;
                else if (not turn.push_back(player)) {
                    azsg = false;  // player got more than 1 turn within an
                                   // n-consecutive plays/sates
                    break;
                }
                // compute next state
                auto action = uAction(new Action);
                for (uint r = 0; r < roles.size(); r++) {
                    auto indx = rand() % moves[r]->size();
                    action->push_back((*moves[r])[indx]);
                }
                state.reset(reasoner.next(*state, *action));
            }
            if (ambigious)
                ambcount++;
            if (reasoner.terminal(*state) or done) {
                if (count and (ambcount / count >= 0.34))
                    azsg = false;
                return;
            }
            if (turn.size()) {
                if (not order.size())
                    order = turn;
                if (not order.equal(turn))
                    azsg = false;  // player got more than 1 turn within an
                                   // n-consecutive plays/sates
            }
            check(std::move(state));
        };

        check(std::move(state));
    }
    void GameAnalyzer::checkZero(const State* init, std::atomic_bool& azsg)
    {
        float sum = INFINITY;
        bool unset = true;
        auto deleter = [&](const State* s) {
            if (s != init)
                delete s;
        };

        while (not done and azsg) {
            uState state(init, deleter);
            while ((not done) and not reasoner.terminal(*state)) {
                auto* action = reasoner.randAction(*state);
                state.reset(reasoner.next(*state, *action));
                delete action;
            }
            if (done)
                break;
            float localSum = 0;
            for (auto&& role : reasoner.roles())
                localSum += reasoner.reward(*role, state.get());

            if (unset) {
                unset = false;
                sum = localSum;
            }

            if (sum != localSum)
                azsg = false;
        }
    }
}  // namespace ares