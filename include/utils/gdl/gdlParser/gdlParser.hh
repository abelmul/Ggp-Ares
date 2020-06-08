#ifndef GDL_PARSER_HH
#define GDL_PARSER_HH
#include "utils/gdl/term.hh"
#include "utils/gdl/variable.hh"
#include "utils/gdl/constant.hh"
#include "utils/gdl/function.hh"
#include "utils/gdl/literal.hh"
#include "utils/game/state.hh"
#include "utils/gdl/clause.hh"
#include "utils/gdl/gdlParser/expressionPool.hh"
#include <iostream>
#include <fstream>
#include <string>
#include <boost/regex.hpp>
#include <boost/algorithm/string.hpp> 
#include <boost/algorithm/string_regex.hpp> 
#include <boost/asio.hpp>
#include <stack>


namespace Ares
{
    
    using namespace std;
    typedef boost::asio::thread_pool thread_pool;
    typedef vector<Term*> Body;
    enum Pstate {NEW, BALANCE,END};
    class GdlParser
    {
    private:
        string preprocess(string& expr);
        Literal* parseFact(vector<string>::iterator start, vector<string>::iterator end);
        void parseRule(Clause* c, vector<string>::iterator start, vector<string>::iterator end);
        void parseBody(Clause* c, vector<string>::iterator start, vector<string>::iterator end);
        void parseTerm(vector<string>::iterator start, vector<string>::iterator end, Body& body);
        void parse(string& expr);
        void getBalanced(vector<string>::iterator& it,vector<string>& p);

        thread_pool* pool;
        KnowledgeBase* base;
        ExpressionPool* exprPool;
        static SpinLock slock;
        static GdlParser* _parser;
        const static vector<Term*>* EMPTY_BODY;
        const static vector<Literal*>* EMPTY_CBODY;
        GdlParser(uint nThreads): pool(new thread_pool(nThreads)){
            exprPool = new ExpressionPool();
        }

    public:
        static GdlParser* getParser(uint nThreads){
            slock.lock();
            if(! _parser)
                _parser = new GdlParser(nThreads);
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
            throw "[*] GdlParser ::Error:: A literal/term Name should only consist of Alpha Numeric characters.";
    }
} // namespace Ares

#endif