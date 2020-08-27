#ifndef HTTP_HANDLER_HH
#define HTTP_HANDLER_HH
#include <string>
// #include <>
#include "ares.hh"

#include <cpprest/http_listener.h>
#include <cpprest/json.h>
#include <mutex>
#include <condition_variable>
#include <functional>
#include <boost/algorithm/string/case_conv.hpp>
#include <atomic>

namespace ares
{
    class HttpHandler
    {
        typedef std::function<std::string(std::reference_wrapper<std::vector<std::string>>)> Hook;
    public:
        HttpHandler(Ares& ares_,std::string url)
        :ares(ares_),listener(url),hooks(3),playing(false)
        {
            using namespace web::http;
            
            //Setup the hooks
            hooks[START] = ([this](std::vector<std::string>& s){ return startHandler(s); });
            hooks[PLAY] = ([this](std::vector<std::string>& s){ return playHandler(s); });
            hooks[STOP] = ([this](std::vector<std::string>& s){ return stopHandler(s); });

            //Start up the server
            listener.support(methods::POST,std::bind(&HttpHandler::handle, this, std::placeholders::_1));
            listener.open()
            .then([&]{cout << "[*] HttpHandler: Ares Server Started on Adress : " << url << "\n";})
            .wait();
        }

        /**
         * Stop accepting requests.
         */
        void shutdown(){ listener.close().wait(); }

        ~HttpHandler() {}

    //  private:
        void handle(web::http::http_request msg);
        /**
         * Handle the start message (http) reply 'BUSY' if playing another game. 
         * Otherwise reply with 'READY' when ready.
         */
        std::string startHandler(std::vector<std::string>&);
        std::string playHandler(std::vector<std::string>&);
        std::string stopHandler(std::vector<std::string>&);

        /**
         * Helper methods to get message type
         */

        inline short type(const std::string& s){
            const auto& str = boost::to_lower_copy(s);
            
            if(  str == "play")
                return PLAY;
            else if(  str == "start")
                return START;
            else if(   str == "stop")
                return STOP;
            
            return -1;
        }
    /**
     * Data 
     */
    private:
        Ares& ares;
        Match currentMatch;
        web::http::experimental::listener::http_listener listener;
        std::vector<Hook> hooks;
        std::atomic_bool playing;

        const ushort START=0;
        const ushort PLAY=1;
        const ushort STOP=2;


    };
} // namespace ares

#endif
