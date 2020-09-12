#ifndef STRATEGY_HH
#define STRATEGY_HH
#include "utils/game/match.hh"
#include "reasoner/reasoner.hh"

namespace ares
{
    class Strategy
    {
    protected:
        Strategy():current(nullptr){}

        Strategy(const Strategy&)=delete;
        Strategy& operator=(const Strategy&)=delete;
        Strategy(const Strategy&&)=delete;
        Strategy& operator=(const Strategy&&)=delete;
        
    public:
        typedef std::pair<move_sptr,uint> move_sptr_seq;
        static void setReasoner(Reasoner* r) {reasoner = r;}

        /**
         * Makes a move based on the current state of the match
         */
        virtual move_sptr_seq operator()(const Match& match,uint) = 0;
        virtual void init()=0;
        virtual void start(const Match& match) = 0;
        virtual void reset() = 0;
        virtual const State& matchState(){ return *current;}
        virtual std::string name() = 0;
        static std::string name(Strategy* s){ return s->name();}
        virtual ~Strategy(){}
    
    protected:
        const State* current;

    /**
     * DATA
     */
    protected:
        static Reasoner* reasoner;
    };


    /**
     * To get all created Strategies to register themselves
     */
    class Registrar
    {
    private:
        static std::unordered_map<std::string, Strategy*> strategies;
        static std::unordered_set<const char*> initd;
    public:
        Registrar(Strategy* strategy) {
            if( not strategy ) return;
            auto name = Strategy::name(strategy);
            if( strategies.find(name) == strategies.end()) 
                strategies[Strategy::name(strategy)] = strategy;
        }
        ~Registrar() { 
        }

        static Strategy& get(const char* name){
            Strategy& st = *strategies[name];
            if( initd.find(name) == initd.end()) st.init();
            return st;
        }

        template <class T>
        friend class RegistrarBase;
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