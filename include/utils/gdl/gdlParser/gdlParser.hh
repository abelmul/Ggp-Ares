#ifndef GDL_PARSER_HH
#define GDL_PARSER_HH
#include "utils/gdl/gdlParser/expressionPool.hh"
#include "utils/gdl/gdlParser/transformer.hh"
#include <iostream>
#include <fstream>
#include <boost/regex.hpp>
#include <boost/algorithm/string_regex.hpp> 
#include <stack>
#include <memory>

namespace Ares
{
    
    using namespace std;
    typedef boost::asio::thread_pool thread_pool;
    typedef vector<Term*> Body;
    enum Pstate {NEW, BALANCE,END};
    struct TokenStream;
    class GdlParser
    {
        friend class Transformer;
    
    private:

        GdlParser(uint nThreads): pool(new thread_pool(nThreads)){
            exprPool = new ExpressionPool();
        }

        void parse(string& expr);
        string preprocess(string& expr);
        const Literal* parseLiteral(vector<string>::iterator& start, const vector<string>::iterator& end,bool p=true);
        void parseRule(Clause* c, vector<string>::iterator start,const vector<string>::iterator end);
        const Literal* _create(stack<pair<string,Body*>>& bodies,bool p=true);

        vector<string> tokens;
        ExpressionPool* exprPool;
        KnowledgeBase* base;
        thread_pool* pool;
        
        //Static members
        static GdlParser* _parser;
        static Transformer* transformer;
        static SpinLock slock;

        //Constants
        const static Body* EMPTY_BODY;
        static ClauseBody* EMPTY_CBODY;

    public:
        //Singleton parser/transformer
        static GdlParser* getParser(uint nThreads){
            slock.lock();
            if(! _parser){
                _parser = new GdlParser(nThreads); 
                transformer = new Transformer(_parser);
            }
            slock.unlock();
            return _parser;
        }

        void parse(KnowledgeBase* base, string& gdl);
        void parseFile(KnowledgeBase* base, string& gdlF);
        ~GdlParser(){
            delete pool;
            delete exprPool;
            delete EMPTY_BODY;
            delete EMPTY_CBODY;
        }
    };
    
    inline void checkValid(string& s){
        if( !isalnum( s[0] )) 
            throw SyntaxError("[*] GdlParser ::Error:: A literal/term Name should only consist of Alpha Numeric characters.");
    }
} // namespace AresAresAres

#endif