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
#include "utils/utils/cfg.hh"

namespace ares
{
    class HttpHandler
    {
        typedef std::function<std::string(std::reference_wrapper<std::vector<std::string>>)> Hook;
    public:
        HttpHandler(Ares& ares_,std::string url);

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
            
            if(   str == "info" )
                return INFO;
            else if(  str == "play")
                return PLAY;
            else if(  str == "start")
                return START;
            else if(   str == "stop" )
                return STOP;
            else if( str == "abort")
                return ABORT;
            return -1;
        }
        void wait(){
            //Just wait
            std::mutex mWait;
            std::condition_variable cvWait;
            std::unique_lock<std::mutex> lk(mWait);
            cvWait.wait(lk);
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
        std::atomic_uint seq;
        std::mutex lock;
        
        const ushort INFO=0;
        const ushort START=1;
        const ushort PLAY=2;
        const ushort STOP=3;
        const ushort ABORT=4;


    };
} // namespace ares

#endif
