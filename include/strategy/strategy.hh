#ifndef STRATEGY_HH
#define STRATEGY_HH
#include "utils/game/match.hh"
#include "reasoner/reasoner.hh"
#include "gameAnalyzer/gameAnalyzer.hh"
namespace ares
{
    class Registrar;
    static Registrar* reg;
    class Strategy
    {
    protected:
        Strategy():current(nullptr){}

        Strategy(const Strategy&)=delete;
        Strategy& operator=(const Strategy&)=delete;
        Strategy(const Strategy&&)=delete;
        Strategy& operator=(const Strategy&&)=delete;
        
    public:
        typedef std::pair<Move*,uint> move_sptr_seq;

        /**
         * Makes a move based on the current state of the match
         */
        virtual move_sptr_seq operator()(const Match& match,uint) = 0;
        virtual void init(Reasoner* r,GameAnalyzer* a){reasoner = r; analyzer = a;};
        virtual void start(const Match& match) = 0;
        virtual void reset() = 0;
        virtual void dump(std::string str="{}"){
            std::ofstream f(cfg.stateDumpF);
            f.setf(std::ios::unitbuf);
            f << str;
            f.close();
        }
        virtual const State& matchState(){ return *current;}
        virtual std::string name() = 0;
        static std::string name(Strategy* s){ return s->name();}
        virtual ~Strategy(){}
    
    /**
     * DATA
     */
    protected:
        const State* current;
        GameAnalyzer* analyzer;
        static Reasoner* reasoner;
    };


    /**
     * To get all created Strategies to register themselves
     */
    class Registrar
    {
    private:
        static std::unordered_map<std::string, Strategy*> strategies;
        static std::unordered_set<std::string> initd;
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
            return st;
        }
        inline static void init(Reasoner* r,GameAnalyzer* a){
            for (auto &&[name,strategy] : strategies)
                if( initd.find(name) == initd.end()){ strategy->init(r,a); initd.insert(name);}
        }
        template <class T>
        friend class RegistrarBase;
    };
    
    template <class T>
    class RegistrarBase
    {
    public:
        RegistrarBase() {reg = &registrar;}
    private:
        static Registrar registrar;
    };
} // namespace ares

#endif