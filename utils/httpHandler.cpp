#include "utils/utils/httpHandler.hh"

namespace ares
{
    using namespace web;
    using namespace web::http;
    using namespace web::http::experimental::listener;


    HttpHandler::HttpHandler(Ares& ares_,std::string url)
    :ares(ares_),listener(url),hooks(5),playing(false),seq(0)
    {
        using namespace web::http;
        
        //Setup the hooks
        hooks[INFO] = [&](std::vector<std::string>& s){
            const char* status = playing ? "busy))" : "available))";
            return std::string("( (name Ares) (status ") + status;  //Return info
        };
        hooks[START] = [this](std::vector<std::string>& s){
            std::unique_lock<std::mutex> lk(lock);
            return startHandler(s); 
        };
        hooks[PLAY] = [this](std::vector<std::string>& s){ return playHandler(s); };
        hooks[STOP] = [this](std::vector<std::string>& s){
            std::unique_lock<std::mutex> lk(lock);
            return stopHandler(s); 
        };
        hooks[ABORT] =[this](std::vector<std::string>& s){ 
            std::unique_lock<std::mutex> lk(lock);
            if( !ares.abortMatch(s[2]) ) return "";
            playing = false; 
            seq = 0;
            return "DONE";
        };

        //Start up the server
        listener.support(methods::POST,std::bind(&HttpHandler::handle, this, std::placeholders::_1));
        listener.open()
        .then([&]{ log("\n[HttpHandler]") << "Ares Server Started on Adress : " << url << "\n";})
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
            if( reply.size() ) msg.reply(status_codes::OK, reply);
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
        if( tokens[2] != ares.currentMatch() )
            return "WRONG MATCH";
        uint cseq = ++seq;
        log("[HttpHandler]") << "Recieved Play message for match : " << tokens[2] << '\n';
        
        log("[HttpHandler]") << "Message Sequence num : " << cseq << '\n';
        tokens.pop_back();
        tokens.erase(tokens.begin(), tokens.begin()+3);

        std::pair<move_sptr,uint> selectedMove;
        if( tokens[0] != "nil" ){
            //Parse the made moves
            const auto& moves = ares.parser.parseSeq(tokens);
            //Get the next move from ares strategy
            selectedMove = ares.makeMove(cseq,moves);
        }
        else
            selectedMove = ares.makeMove(cseq);
        // Make sure Another play message hasn't come before replying
        bool valid =  selectedMove.second == seq and selectedMove.first ;
        if( not valid ){
            if(selectedMove.second != seq )
                logerr("[HttpHandler]") << "Strategy returned invalid move.\n\tcurrent seq="<<seq << ", returned seq = " << selectedMove.first <<"\n";
            else
                logerr("[HttpHandler]") << "Strategy returned invalid move. Returned move is null.\n";
        }
        return valid ? selectedMove.first->to_string() : "";
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
        delete moves;
        //At the end
        playing = false;
        seq = 0;
        return "DONE";
    }

    
} // namespace ares
