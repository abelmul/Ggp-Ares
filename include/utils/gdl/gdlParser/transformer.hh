#ifndef TRANSFORMER_HH
#define TRANSFORMER_HH
#include <string>
#include <vector>
#include "utils/gdl/clause.hh"
#include "utils/game/match.hh"
#include "utils/utils/exceptions.hh"
#include <functional>
#include <boost/asio.hpp>

namespace ares
{
    using namespace std;
    class GdlParser;
    struct TokenStream;

    class Transformer
    {
        friend class GdlParser;

    private:
        // Managed/Owned by GdlParser
        Transformer(GdlParser* p):parser(p) {}
        /**
         * Applies transformations to the clause body. The transformations are as follows.
         * ~~L to L
         *  A <= A0,..,Ai-1,(W or V),..An      to      A <= A0,..,Ai-1,W,..An and A <= A0,..,Ai-1,V,..An
         *  A <= A0,..,Ai-1,~(W and V),..An    to      A <= A0,..,Ai-1,~W,..An and A <= A <= A0,..,Ai-1,~V,..An
         *  A <= A0,..,Ai-1,~(W or V),..An     to      A <= A0,..,Ai-1,~W,~V,..An
         */
        void applyTransformations(Clause* c, KnowledgeBase* base,unique_ptr<TokenStream> stream);
        /**
         * Get and expression with a balanced parentheses.
         */
        void getBalanced(vector<string>::iterator& it,const vector<string>::iterator& end);
        void parseFml(Clause* c, KnowledgeBase* base,TokenStream& stream);
        void transformNot(Clause* c, vector<string>::iterator& it, TokenStream& stream);
        static Transformer* create(GdlParser* p){
            static Transformer transformer(p);
            return &transformer;
        }


        ~Transformer() {}
        /**Data**/
        GdlParser* parser;
    };

    struct TokenStream
    {
        /**
         * Don't Need 'em
         */
        TokenStream& operator=(const TokenStream& s) = delete;
        TokenStream(const TokenStream& s) = delete;
        /****/

        /**
         * Just take the reference.
         */
        TokenStream(
            vector<string>& ts,
            vector<string>::iterator cur,      //current "position" of the stream    
            vector<string>::iterator nxt,      //Next "position" 
            vector<string>::iterator end       //End of the stream    
        )
        :tokens(ts),current_(cur),end_(end)
        {
            setNext(nxt);
        }
        /**
         * If Duplication of the underlying stream(vector) is necessary.
         */
        TokenStream(vector<string>* ts):tokens(ref(*ts)){
            tokens_ = ts;
            current_ = tokens.begin();   //current "position" of the stream   
            next = current_;             //Next "position"
            end_ = tokens.end();         //End of the stream
        }
        ~TokenStream(){
            if(tokens_ ) delete tokens_;
        }

        TokenStream& operator++(){
            if ( next <= current_)
                current_++;
            else
                current_ = next;

            return *this;
        }
        string& operator*(){
            return *current_;
        }
        vector<string>::iterator& current(){ return current_;}
        void current(vector<string>::iterator c){ current_ = c;}
        void setNext(vector<string>::iterator n){
            if( n > next)
                next = n;
        }
        const vector<string>::iterator& getNext(){return next;}
        void replaceData(vector<string>* data_){
            if(tokens_ ) delete tokens_;
            tokens_ = data_;
            tokens = ref(*tokens_);
            current_ = data_->begin();
            next = current_;
            end_ = data_->end();
        }

        const vector<string>::iterator end(){ return end_;}
        vector<string>&  data() { return tokens;}
        
    private:
        vector<string>& tokens;
        vector<string>* tokens_ = nullptr;
        vector<string>::iterator current_;
        vector<string>::iterator next;
        vector<string>::iterator end_;

    };
} // namespace ares

#endif