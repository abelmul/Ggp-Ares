#include "utils/utils/httpHandler.hh"

namespace ares
{
    using namespace web;
    using namespace web::http;
    using namespace web::http::experimental::listener;

    void HttpHandler::handle(http_request msg){
        std::cout << "[HttpHandler] Got a Post Message...\n";
        
        //Determine the message type
        msg.extract_string()
        .then([&](pplx::task<string> task){
            auto sMsg = task.get();
            std::vector<std::string> tokens;
            ares.parser->tokenize(sMsg.c_str(), tokens);
            auto t = type(tokens[1]);
            if( t == -1 )
            {
                msg.reply(status_codes::BadRequest);
                return;
            }
            //One the the start, play, or stop messages handle them accordingly.
            auto& hook = hooks[t];
            auto reply = hook(tokens);
            std::cout << "[HttpHandler] HttpReply : " << reply << "...\n";
            msg.reply(status_codes::OK, reply);
        }).wait();
    }
    /**
     * Handles the start http message.
     * (START <MATCHID> <ROLE> <DESCRIPTION> <STARTCLOCK> <PLAYCLOCK>)
     */
    std::string HttpHandler::startHandler(std::vector<std::string>& tokens){
        if( playing.exchange(true) )
            return "BUSY";
        
        Match match;
        
        //id and role
        match.matchId = tokens[2];
        auto role = tokens[3];

        //play clock and start clock
        match.plyClck = std::atoi((tokens.end()-2)->c_str());
        match.strtClck = std::atoi((tokens.end()-3)->c_str());

        //Get the gdl description, <DESCRIPTION>
        tokens.erase(tokens.begin(),tokens.begin()+4);
        tokens.erase(tokens.end()-3, tokens.end());

        match.game = new Game();
        ares.parser->parse(match.game, tokens);

        match.state = match.game->getInit();

        std::cout << "[HttpHandler] Starting a new match\n";
        std::cout << "[HttpHandler] Id : " << match.matchId << "\n";
        std::cout << "[HttpHandler] Role : " << role << "\n";
        std::cout << "[HttpHandler] Play Clock : " << match.plyClck << "\n";
        std::cout << "[HttpHandler] Start Clock : " << match.strtClck << "\n";
        std::cout << "[HttpHandler] Gdl : \n" << match.game->toString() << "\n";

        //Need to analyze the game and so on.
        ares.startMatch(match,role);

        return "READY";
    }
    /**
     * Handles the start http message.
     * (PLAY <MATCHID> (<A1> <A2> ... <An>))
     */
    std::string HttpHandler::playHandler(std::vector<std::string>& tokens){
        static visualizer viz;
        std::cout << "[HttpHandler] Recieved Play message for match : " << tokens[2] << '\n';

        if( tokens[2] != ares.currentMatch() )
            return "WRONG MATCH";
        tokens.pop_back();
        tokens.erase(tokens.begin(), tokens.begin()+3);

        std::cout << "[HttpHandler] " ;
        for (auto &&i : tokens)
            std::cout << i << " ";
        
        std::cout << "\n";
        cnst_term_sptr selectedMove;
        if( tokens[3] != "nil" ){
            //Parse the made moves
            const auto& moves = ares.parser->parseSeq(tokens);
            //Get the next move from ares strategy
            selectedMove = ares.makeMove(moves);
        }
        else
            selectedMove = ares.makeMove();
        viz.draw( ares.getMatchState(),false);
        return selectedMove->to_string();
    }
    std::string HttpHandler::stopHandler(std::vector<std::string>& tokens){
        ares.stopMatch();
        //At the end
        playing = false;
        return "DONE";
    }
} // namespace ares
