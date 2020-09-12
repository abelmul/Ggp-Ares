#include "utils/utils/httpHandler.hh"

namespace ares
{
    using namespace web;
    using namespace web::http;
    using namespace web::http::experimental::listener;

    inline std::ostream& log(const std::string& msgs);

    HttpHandler::HttpHandler(Ares& ares_,std::string url)
    :ares(ares_),listener(url),hooks(5),playing(false)
    {
        using namespace web::http;
        
        //Setup the hooks
        hooks[INFO] = [&](std::vector<std::string>& s){
            const char* status = playing ? "busy))" : "available))";
            return std::string("( (name Ares) (status ") + status;  //Return info
        };
        hooks[START] = [this](std::vector<std::string>& s){ return startHandler(s); };
        hooks[PLAY] = [this](std::vector<std::string>& s){ return playHandler(s); };
        hooks[STOP] = [this](std::vector<std::string>& s){ return stopHandler(s); };
        hooks[ABORT] =[this](std::vector<std::string>& s){ 
            if( !ares.abortMatch(s[1]) ) return "";
            playing = false; 
            return "DONE";
        };

        //Start up the server
        listener.support(methods::POST,std::bind(&HttpHandler::handle, this, std::placeholders::_1));
        listener.open()
        .then([&]{ log("[HttpHandler]") << "Ares Server Started on Adress : " << url << "\n";})
        .wait();
    }
    void HttpHandler::handle(http_request msg){
        //Determine the message type
        msg.extract_string()
        .then([&](pplx::task<string> task){
            auto sMsg = task.get();
            std::vector<std::string> tokens;
            ares.parser.tokenize(sMsg.c_str(), tokens);
            auto t = type(tokens[1]);
            //Don't log pings.
            if( cfg.debug or t != INFO )  log("[HttpHandler]") << "Got a Post Message : " << sMsg << "\n"; 
            if( t == -1 )
            {
                msg.reply(status_codes::BadRequest);
                return;
            }
            //One of the info, start, play, or stop messages; handle them accordingly.
            auto& hook = hooks[t];
            auto reply = hook(tokens);
            if( cfg.debug or t != INFO ) log("[HttpHandler]") << "HttpReply : " << reply << "\n";
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
        ares.parser.parse(match.game, tokens);

        match.state = match.game->getInit();


        log("[HttpHandler]") << "Starting a new match\n";
        log("[HttpHandler]") << "Id : " << match.matchId << "\n";
        log("[HttpHandler]") << "Role : " << role << "\n";
        log("[HttpHandler]") << "Play Clock : " << match.plyClck << "\n";
        log("[HttpHandler]") << "Start Clock : " << match.strtClck << "\n";

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
        log("[HttpHandler]") << "Recieved Play message for match : " << tokens[2] << '\n';

        if( tokens[2] != ares.currentMatch() )
            return "WRONG MATCH";
        
        tokens.pop_back();
        tokens.erase(tokens.begin(), tokens.begin()+3);

        cnst_term_sptr selectedMove;
        if( tokens[3] != "nil" ){
            //Parse the made moves
            const auto& moves = ares.parser.parseSeq(tokens);
            //Get the next move from ares strategy
            selectedMove = ares.makeMove(moves);
        }
        else
            selectedMove = ares.makeMove();
        // viz.draw( ares.getMatchState(),false);
        return selectedMove->to_string();
    }
    /**
     *(STOP <MATCHID> (<A1> <A2> ... <An>))
     */
    std::string HttpHandler::stopHandler(std::vector<std::string>& tokens){
        if( tokens[2] != ares.currentMatch() )
            return "WRONG MATCH";
        
        tokens.pop_back();
        tokens.erase(tokens.begin(), tokens.begin()+3);

        const auto& moves = ares.parser.parseSeq(tokens);
        ares.stopMatch(moves);
        //At the end
        playing = false;
        return "DONE";
    }

    
} // namespace ares
