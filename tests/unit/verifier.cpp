#include <iostream>
#include <fstream>
#include <boost/property_tree/ptree.hpp>
#include <boost/property_tree/json_parser.hpp>
#include <boost/foreach.hpp>
#include <algorithm>
#include "ares.hh"
#include "static.hh"
#include "strategy/random.hh"
#include "runner.hh"

// #define GAME_DIR "tests/ggp.org/games/"
#define CHESS_MATCHES "tests/unit/selectedGames/chess_matches.json"
#define SELECTED_MATCHES "tests/unit/selectedGames/matches.json"
#define SELECTED_DIR "tests/unit/selectedGames/"
#define CHESS_GAME "tests/unit/selectedGames/chess.kif"
#define MAPPINGS  "tests/unit/selectedGames/mapping.json"

#define N 41775
#define SAMPLE 10

using namespace boost::property_tree;

/**
 * Iterates through the states and made moves of a recorded match on ggp.org 
 * and asserts that the (locally) computed states and moves are equal.
 */
void verifyMatches(std::vector<const char*>);
void simulateMatch(ptree& match,ares::Reasoner& reasoner);

std::vector<std::vector<const ares::Term *>> getMove(ptree& pt);
std::vector<std::vector<const ares::Term *>*> getState(ptree& pt);

void writeOutMatch();
ptree getMatch(std::vector<uint>& selected);
namespace ares{
    Cfg cfg("./ares.cfg.json");
    GdlParser* parser;
};
using namespace ares;
void setup(){
    using namespace ares;
    srand(time(NULL));
    Ares::setMem(&mempool);

    Body::mempool =  &mempool;
    SuffixRenamer::setPool(Ares::memCache);
}

std::ofstream out;
int main(){
    setup();
    Runner runner;
    runner.iter =1;
    add_test(runner,[&]{verifyMatches({CHESS_MATCHES,SELECTED_MATCHES});});
    runner();
    return 0;
}

void verifyMatches(std::vector<const char*> matchFiles){
    ptree mappings;
    read_json(MAPPINGS, mappings);

    cfg.proverThreads =1;
    Prover& prover = Prover::create();
    ClauseCB::prover = &prover;

    parser = &GdlParser::create(mempool.getCache());
    //The reasoner
    Reasoner& reasoner(Reasoner::create(*parser, Prover::create(), *mempool.getCache()));
    Ares::create(Registrar::get("Random"),reasoner);
    
    for (auto &&matchF : matchFiles)
    {
        ptree pt;
        read_json(matchF,pt);
        BOOST_FOREACH(auto& match1,pt){
            //match.first == chess0
            auto& match = match1.second;
            Game* g(new Game());
            string name;
            if( match1.first.find("chess") != string::npos )
                name = string(CHESS_GAME);
            
            else
                name = SELECTED_DIR + mappings.get<string>(ptree::path_type(match1.first,'|'));
            
            std::cout << "Using GDl file : " << name << "\n";
            std::cout << "Verifying Reasoner with Match: " << match1.second.get<string>("url") << "\n";
            parser->parse(g,name);    
            reasoner.reset(g);
            simulateMatch(match, reasoner);
            log("Successfuly Verified Reasoner With Match: ") << match1.second.get<string>("url") << "\n";
        }
    }
    
}

void simulateMatch(ptree& match,ares::Reasoner& reasoner){
    auto pmatchStates = match.get_child("data.states");
    auto pmatchMoves = match.get_child("data.moves");

    auto matchStates = getState(pmatchStates);
    auto matchMoves = getMove(pmatchMoves);

    const auto& roles = reasoner.roles();
    
    const State *computedState = &reasoner.init();
    uint step =0;
    for (auto &&moves : matchMoves)
    {
        auto& lstMatchState = matchStates[step];
        State matchState;
        assert_true(moves.size() == roles.size());
        for (auto &&s : *lstMatchState){
            PoolKey key{Namer::TRUE, new Body{s}};
            auto true_ = Ares::memCache->getAtom(key);
            matchState.add(Namer::TRUE, new Clause(true_,new ClauseBody(0)));
        }
        //assert that the computed state is the same as the matches state.
        assert_true( matchState == (*computedState));
        //Compute the legal moves.
        for (size_t i = 0; i < moves.size(); i++)
        {
            //player i made moves[i]
            auto* computedMoves = reasoner.moves(*computedState, *roles[i]);
            //assert that the taken move has been computed
            auto it = find(computedMoves->begin(), computedMoves->end(), moves[i]);
            assert_true( it != computedMoves->end() );
            delete computedMoves;
        }
        //Compute the next state.
        auto* prev =computedState;
        computedState = reasoner.next(*prev,*(cnst_term_container*)&moves);
        if( prev!= &reasoner.init()) delete prev;
        step++;
    }
    //Verify the terminal state.
    State matchState;
    auto last = matchStates.size()-1;
    for (auto &&s : *matchStates[last]){
        PoolKey key{Namer::TRUE, new Body{s}};
        auto true_ = Ares::memCache->getAtom(key);
        matchState.add(Namer::TRUE, new Clause(true_,new ClauseBody(0)));
    }
    assert_true( matchState == (*computedState));
    if( computedState!= &reasoner.init()) delete computedState;
    for (auto &&state : matchStates)
        delete state;
    
}
/**
 * Get the ith moves of the match
 */
std::vector<std::vector<const Term*>> getMove(ptree& pt){
    std::vector<std::vector<const Term*>> moves;
    if( moves.size() == 0){
        BOOST_FOREACH(auto& smoves, pt){
            std::vector<const Term*> moves_i;
            BOOST_FOREACH(auto& move, smoves.second){
                auto move_s = move.second.get_value<std::string>();
                if( move_s[0] == '(' )
                    moves_i.push_back(parser->parseFn(move_s.c_str()));
                else
                    moves_i.push_back(Ares::memCache->getConst(Namer::id(move_s)));
            }
            moves.push_back(moves_i);
        }
    }
    return moves;
}
/**
 * Get the ith state of the match
 */
std::vector<std::vector<const Term*>*> getState(ptree& pt){
    std::vector<std::vector<const Term*>*> states;
    if( states.size() ==0 ){
        BOOST_FOREACH(ptree::value_type& x, pt){
            auto seq = parser->parseSeq(x.second.get_value<std::string>().c_str());
            states.push_back(seq);
        }
    }
    return states;
}

void writeOutMatch(){
    out = std::ofstream(SELECTED_MATCHES,std::ios::app);
    out.setf(std::ios::unitbuf);
    std::cout.setf(std::ios::unitbuf);
    out << "{ ";
    //Get a match
    fflush(NULL);
    std::vector<uint> allMatches(N);
    std::iota(allMatches.begin(), allMatches.end(), 0);

    std::random_shuffle(allMatches.begin(), allMatches.end());

    std::vector<uint> selected(allMatches.begin(), allMatches.begin() + SAMPLE);
    for (auto &&i : selected)
        std::cout << "Selecting matche " << i <<"\n";
    
    auto match = getMatch(selected);
    if( not match.size() ){
        std::cout << "Couldn't find a match with game name " << "chess" << "\n";
        return;
    }
    out <<"}";
}

ptree getMatch(std::vector<uint>& selected){
    uint i=0;
    using namespace std;
    string name("tests/ggp.org/matchesFrom2014");
    ifstream matches(name);    
    ptree pt;
    for (string line; getline(matches, line);++i){
        std::stringstream ss;
        std::string sep = "";
        ss << line;
        read_json(ss, pt);
        try
        {
            auto name = pt.get<string>("data.gameMetaURL");
            auto it = std::find(selected.begin(),selected.end(), i);
            if( it != selected.end()){
                if( not pt.get<bool>("data.isCompleted") ) {
                    uint j=1;
                    while ( std::find(selected.begin(),selected.end(),i+j) != selected.end()) j++;
                    *it = i+j;
                    continue;
                };
                cout << name <<'\n';
                out << sep << "\"" << name << " \": " << line ;
                sep = ",";
            }
        }
        catch(const std::exception& e){
            cout << e.what() <<"\n";
            continue;
        }
    }
    return pt;
}