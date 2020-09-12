#ifndef STRATEGY_HH
#define STRATEGY_HH
#include "utils/game/match.hh"
#include "reasoner/reasoner.hh"

namespace ares
{
    class Strategy
    {
    protected:
        Strategy(){}

        Strategy(const Strategy&)=delete;
        Strategy& operator=(const Strategy&)=delete;
        Strategy(const Strategy&&)=delete;
        Strategy& operator=(const Strategy&&)=delete;
        
    public:
        virtual move_sptr operator()(const Match& match, Reasoner& reasoner) = 0;
        virtual std::string name() = 0;
        static std::string name(Strategy* s){ return s->name();}
        virtual ~Strategy(){}
    };


    /**
     * To get all created Strategies to register themselves
     */
    class Registrar
    {
    private:
        static std::unordered_map<std::string, Strategy*> strategies;
    public:
        Registrar(Strategy* strategy) {
            if( not strategy ) return;

            auto name = Strategy::name(strategy);
            if( strategies.find(name) == strategies.end()) 
                strategies[Strategy::name(strategy)] = strategy;
        }
        ~Registrar() { 
            for (auto &&[name,strategy] : strategies)
                delete strategy;   
        }

        static Strategy& get(const char* name){return *strategies[name];}
    };
    
    template <class T>
    class RegistrarBase
    {
    private:
    /* data */
    public:
    RegistrarBase(/* args */) {&registrar;}

    static Registrar registrar;
    };
} // namespace ares

#endif