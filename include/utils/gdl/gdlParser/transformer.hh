#ifndef TRANSFORMER_HH
#define TRANSFORMER_HH
#include <string>
#include <vector>
#include "utils/gdl/clause.hh"
#include "utils/game/match.hh"
#include "utils/gdl/gdlParser/exceptions.hh"
#include <functional>
#include <boost/asio.hpp>

namespace Ares
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
        void applyTransformations(Clause* c, unique_ptr<TokenStream> stream);
        /**
         * Get and expression with a balanced parentheses.
         */
        void getBalanced(vector<string>::iterator& it,const vector<string>::iterator& end);
        void parseFml(Clause* c, TokenStream& stream);
        void _transformNot(Clause* c, vector<string>::iterator& it, TokenStream& stream);
        static Transformer* getTransformer(GdlParser* p){
            slock.lock();
            if(not _t) _t = new Transformer(p);
            slock.unlock();
            return _t;
        }


        ~Transformer() {}
        /**Data**/
        static Transformer* _t;
        static SpinLock slock;
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
        :tokens(ts),_current(cur),_end(end)
        {
            setNext(nxt);
        }
        /**
         * If Duplication of the underlying stream(vector) is necessary.
         */
        TokenStream(vector<string>* ts):tokens(ref(*ts)){
            _tokens = ts;
            _current = tokens.begin();   //current "position" of the stream   
            next = _current;             //Next "position"
            _end = tokens.end();         //End of the stream
        }
        ~TokenStream(){
            if(_tokens ) delete _tokens;
        }

        TokenStream& operator++(){
            if ( next <= _current)
                _current++;
            else
                _current = next;

            return *this;
        }
        string& operator*(){
            return *_current;
        }
        vector<string>::iterator& current(){ return _current;}
        void current(vector<string>::iterator c){ _current = c;}
        void setNext(vector<string>::iterator n){
            if( n > next)
                next = n;
        }
        const vector<string>::iterator& getNext(){return next;}
        void replaceData(vector<string>* _data){
            if(_tokens ) delete _tokens;
            _tokens = _data;
            tokens = ref(*_tokens);
            _current = _data->begin();
            next = _current;
            _end = _data->end();
        }

        const vector<string>::iterator end(){ return _end;}
        vector<string>&  data() { return tokens;}
        
    private:
        vector<string>& tokens;
        vector<string>* _tokens = nullptr;
        vector<string>::iterator _current;
        vector<string>::iterator next;
        vector<string>::iterator _end;

    };
} // namespace Ares

#endif