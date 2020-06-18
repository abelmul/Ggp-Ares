#ifndef GDL_PARSER_HH
#define GDL_PARSER_HH
#include "utils/memory/expressionPool.hh"
#include "utils/gdl/gdlParser/transformer.hh"
#include <iostream>
#include <fstream>
#include <boost/regex.hpp>
#include <boost/algorithm/string_regex.hpp> 
#include <stack>
#include <memory>

namespace ares
{
    
    using namespace std;
    typedef boost::asio::thread_pool thread_pool;
    enum Pstate {NEW, BALANCE,END};
    struct TokenStream;
    class GdlParser
    {
        friend class Transformer;
    
    private:

        GdlParser(uint nThreads): pool(new thread_pool(nThreads)), exprPool(new ExpressionPool()){}

        void parse(string& expr);
        string preprocess(string& expr);
        cnst_lit_sptr parseLiteral(vector<string>::iterator& start, const vector<string>::iterator& end,bool p=true);
        void parseRule(Clause* c, vector<string>::iterator start,const vector<string>::iterator end);
        cnst_lit_sptr _create(stack<pair<string,Body*>>& bodies,bool p=true);
        string removeComments(string gdl);

        vector<string> tokens;
        KnowledgeBase* base;
        thread_pool* pool;
        ExpressionPool* exprPool;
        
        //Static members
        static GdlParser* _parser;
        static Transformer* transformer;
        static SpinLock slock;
        

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
        ExpressionPool* getExpressionPool(){ return exprPool;}
        void parse(KnowledgeBase* base, string& gdl);
        void parseFile(KnowledgeBase* base, string& gdlF);
        cnst_lit_sptr parseQuery(const char * query);
        ~GdlParser(){
            delete pool;
            delete exprPool;
        }
    };
    
    inline void checkValid(string& s){
        if( !isalnum( s[0] )) 
            throw SyntaxError("[*] GdlParser ::Error:: A literal/term Name should only consist of Alpha Numeric characters.");
    }
} // namespace AresAresAres

#endif