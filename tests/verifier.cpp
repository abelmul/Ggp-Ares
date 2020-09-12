#include <iostream>
#include <fstream>
#include <boost/property_tree/ptree.hpp>
#include <boost/property_tree/json_parser.hpp>
#include <boost/foreach.hpp>
#include "ares.hh"

#define GAME_DIR "tests/ggp.org/games/"
#define CHESS_MATCHES "tests/ggp.org/chess_matches.json"
#define CHESS_GAME "tests/ggp.org/games/chess.kif"

using namespace boost::property_tree;
ptree getMatch(const char* name_ish);
std::vector<std::vector<ares::cnst_term_sptr>> getMove(ptree& pt);
std::vector<std::vector<ares::cnst_term_sptr>> getState(ptree& pt);
void verifyChessMatch();
namespace ares{
    Ares* aresP;
    Cfg cfg("./ares.cfg.json");
    GdlParser* parser;
};
using namespace ares;
void setup(){
    using namespace ares;
    srand(time(NULL));
    // aresP = new Ares(new MemoryPool(100,100,std::vector<std::pair<arity_t,uint>>()));
    Body::mempool = ClauseBody::mempool = aresP->mempool;
    Term::null_term_sptr = nullptr;
    Term::null_literal_sptr = nullptr;
    SuffixRenamer::setPool(aresP->memCache);
}

std::ofstream out;
int main(int argc, char const *argv[]){
    setup();
    verifyChessMatch();
}

void verifyChessMatch(){
    
    ptree pt;
    read_json(CHESS_MATCHES,pt);
    auto match1 = pt.get_child("chess0");
    parser = GdlParser::getParser(1,aresP->memCache);
    Game* g(new Game());
    string chess(CHESS_GAME);
    parser->parse(g,chess);
    cfg.proverThreads =1;
    Prover* prover = Prover::getProver(g);
    ClauseCB::prover = prover;
    Reasoner reasoner(*parser,prover,*aresP->memCache,g);

    auto pmatchStates = match1.get_child("data.states");
    auto pmatchMoves = match1.get_child("data.moves");

    auto matchStates = getState(pmatchStates);
    auto matchMoves = getMove(pmatchMoves);

    const auto& roles = reasoner.getRoles();
    
    const State *computedState = &reasoner.getInit();
    uint step =0;
    for (auto &&moves : matchMoves)
    {
        auto& lstMatchState = matchStates[step];
        State matchState;
        assert(moves.size() == roles.size());
        for (auto &&s : lstMatchState){
            PoolKey key{Namer::TRUE, new Body{s}};
            auto true_ = aresP->memCache->getLiteral(key);
            matchState.add(Namer::TRUE, new Clause(true_,new ClauseBody(0)));
        }
        //Assert that the computed state is the same as the matches state.
        assert( matchState == (*computedState));
        //Compute the legal moves.
        for (size_t i = 0; i < moves.size(); i++)
        {
            //player i made moves[i]
            auto* computedMoves = reasoner.legalMoves(*computedState, *roles[i]);
            //Assert that the taken move has been computed
            auto it = find(computedMoves->begin(), computedMoves->end(), moves[i]);
            assert( it != computedMoves->end() );
            delete computedMoves;
        }
        //Compute the next state.
        auto* prev =computedState;
        computedState = reasoner.getNext(*prev,*(cnst_term_container*)&moves);
        if( prev!= &reasoner.getInit()) delete prev;
        step++;
    }
    //Verify the terminal state.
    State matchState;
    auto last = matchStates.size()-1;
    for (auto &&s : matchStates[last]){
        PoolKey key{Namer::TRUE, new Body{s}};
        auto true_ = aresP->memCache->getLiteral(key);
        matchState.add(Namer::TRUE, new Clause(true_,new ClauseBody(0)));
    }
    assert( matchState == (*computedState));
    if( computedState!= &reasoner.getInit()) delete computedState;
}
/**
 * Get the ith moves of the match
 */
std::vector<std::vector<ares::cnst_term_sptr>> getMove(ptree& pt){
    static std::vector<std::vector<ares::cnst_term_sptr>> moves;
    if( moves.size() == 0){
        BOOST_FOREACH(auto& smoves, pt){
            std::vector<ares::cnst_term_sptr> moves_i;
            BOOST_FOREACH(auto& move, smoves.second){
                auto move_s = move.second.get_value<std::string>();
                std::cout << move_s << "\n";
                if( move_s[0] == '(' )
                    moves_i.push_back(parser->parseFn(move_s.c_str()));
                else
                    moves_i.push_back(aresP->memCache->getConst(Namer::id(move_s)));
            }
            moves.push_back(moves_i);
        }
    }
    return moves;
}
/**
 * Get the ith state of the match
 */
std::vector<std::vector<ares::cnst_term_sptr>> getState(ptree& pt){
    static std::vector<std::vector<ares::cnst_term_sptr>> states;
    if( states.size() ==0 ){
        BOOST_FOREACH(ptree::value_type& x, pt){
            auto seq = parser->parseSeq(x.second.get_value<std::string>().c_str());
            states.push_back(seq);
        }
    }
    return states;
}
void writeOutMatch(){
    out = std::ofstream(CHESS_MATCHES,std::ios::app);
    out.setf(std::ios::unitbuf);
    std::cout.setf(std::ios::unitbuf);
    out << "{ ";
    //Get a match
    std::cout << "Looking for match with game name " << "\n";
    fflush(NULL);
    auto match = getMatch("chess");
    if( not match.size() ){
        std::cout << "Couldn't find a match with game name " << "chess" << "\n";
        return;
    }
    out <<"}";
}
/**
 * Get a match which contains name_ish in its name.
 */
ptree getMatch(const char* name_ish){
    uint i=0;
    using namespace std;
    string name("tests/ggp.org/matchesFrom2014");
    ifstream matches(name);    
    ptree pt;
    for (string line; getline(matches, line) and i< 10;){
        std::stringstream ss;
        std::string sep = "";
        ss << line;
        read_json(ss, pt);
        try
        {
            auto name = pt.get<string>("data.gameMetaURL");
            if( name.find("chess") != name.npos ){
                if( not pt.get<bool>("data.isCompleted") ) continue;
                cout << name <<'\n';
                out << sep << "\"" << name << " \": " << line ;
                sep = ",";
                i++;
            }
        }
        catch(const std::exception& e){
            cout << e.what() <<"\n";
            continue;
        }
    }
    return pt;
}

namespace ares
{
    Prover* ClauseCB::prover;
    MemCache* Ares::memCache = nullptr;
    MemoryPool* Ares::mempool = nullptr;
    
    //Initialize static members of term
    cnst_term_sptr     Term::null_term_sptr(nullptr);
    cnst_lit_sptr     Term::null_literal_sptr(nullptr);
    
    //Namer static
    std::unordered_map<ushort, std::string> Namer::vIdName;
    std::unordered_map<std::string, ushort> Namer::vNameId;
    
    std::unordered_map<ushort, std::string> Namer::idName;
    std::unordered_map<std::string, ushort> Namer::nameId;

    /**
     * Reserve ids for known keywords.
     */
    const ushort Namer::ROLE = Namer::registerName(std::string("role"));
    const ushort Namer::INIT = Namer::registerName(std::string("init"));
    const ushort Namer::LEGAL = Namer::registerName(std::string("legal"));
    const ushort Namer::NEXT = Namer::registerName(std::string("next"));
    const ushort Namer::TRUE = Namer::registerName(std::string("true"));
    const ushort Namer::DOES = Namer::registerName(std::string("does"));
    const ushort Namer::DISTINCT = Namer::registerName(std::string("distinct"));
    const ushort Namer::GOAL = Namer::registerName(std::string("goal"));
    const ushort Namer::TERMINAL = Namer::registerName(std::string("terminal"));
    const ushort Namer::INPUT = Namer::registerName(std::string("input"));
    const ushort Namer::BASE = Namer::registerName(std::string("base"));
    const ushort Namer::X = Namer::registerVname(std::string("?x"));
    const ushort Namer::R = Namer::registerVname(std::string("?r"));
    
    template<class T>
    MemoryPool* _Body<T>::mempool =nullptr;

    std::ostream& operator<<(std::ostream& os, const Cfg& cfg){
        os << cfg.str();
        return os;
    }
} // namespace ares