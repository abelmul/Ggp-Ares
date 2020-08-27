#ifndef GDL_PARSER_HH
#define GDL_PARSER_HH
#include "utils/memory/memCache.hh"
#include "utils/gdl/gdlParser/transformer.hh"
#include  "utils/memory/namer.hh"
#include <iostream>
#include <fstream>
#include <boost/regex.hpp>
#include <boost/algorithm/string_regex.hpp> 
#include <stack>
#include <memory>
#include <functional>

namespace ares
{
    
    using namespace std;
    typedef boost::asio::thread_pool thread_pool;
    enum Pstate {NEW, BALANCE,END};
    struct TokenStream;
    class GdlParser
    {
        typedef std::function<cnst_st_term_sptr(std::reference_wrapper<PoolKey>)> Creator;
        friend class Transformer;
    
    private:

        GdlParser(uint nThreads, MemCache* mem): pool(new thread_pool(nThreads)), memCache(mem){}

        void parse(vector<string>& tokens,KnowledgeBase* base);
        string preprocess(string& expr);
        /**
         * Parse structured terms -- Functions and literals.
         */
        cnst_st_term_sptr parseSterm(vector<string>::iterator& start, const vector<string>::iterator& end,const Creator&,bool p=true);
        cnst_st_term_sptr parseSterm(const char*,const Creator& crtr);
        /**
         * Just a wrapper function.
         */
        inline cnst_lit_sptr parseLiteral(vector<string>::iterator& start, const vector<string>::iterator& end,bool p=true){
            auto crtr = [=](PoolKey& k){ return memCache->getLiteral(k);};
            auto st = parseSterm(start, end, crtr,p);
            return *(cnst_lit_sptr*)&st;
        }
        inline cnst_fn_sptr parseFn(vector<string>::iterator& start, const vector<string>::iterator& end){
            auto crtr = [=](PoolKey& k){ return memCache->getFn(k);};
            auto st = parseSterm(start,end,crtr);
            return *(cnst_fn_sptr*)&st;
        }
        /**
         * Parse rules of the form A<-A1,..,An
         */
        void parseRule(Clause* c,KnowledgeBase* base,vector<string>& tokens, vector<string>::iterator start,const vector<string>::iterator end);
        cnst_st_term_sptr _create(stack<pair<string,Body*>>& bodies,const Creator&,bool p=true);
        string removeComments(string gdl);

        
        thread_pool* pool;
        MemCache* memCache;
        
        //Static members
        static GdlParser* _parser;
        static Transformer* transformer;
        static SpinLock slock;
        

    public:
        //Singleton parser/transformer
        static GdlParser* getParser(uint nThreads,MemCache* mem){
            slock.lock();
            if(! _parser){
                _parser = new GdlParser(nThreads,mem); 
                transformer = new Transformer(_parser);
            }
            slock.unlock();
            return _parser;
        }
        /**
         * Parse from a gdl file which is not preprocessed.
         */
        void parse(KnowledgeBase* base, string& gdlF);
        /**
         * parse a preproccessed gdl already tokenized.
         */
        void parse(KnowledgeBase* base, vector<string>& tokens);
        /**
         * Just a wrapper function.
         * @param fn a string representing a function.
         * @returns a function object.
         */
        inline cnst_lit_sptr parseQuery(const char * query){
            auto crtr = [=](PoolKey& k){ return memCache->getLiteral(k);};
            auto st = parseSterm(query,crtr);
            return *(cnst_lit_sptr*)&st;
        }
        /**
         * Just a wrapper function for ease of use.
         * @param fn a string representing a function.
         * @returns a function object.
         */
        inline cnst_fn_sptr parseFn(const char* fn){
            auto crtr = [=](PoolKey& k){ return memCache->getFn(k);};
            auto st = parseSterm(fn,crtr);
            return *(cnst_fn_sptr*)&st;
        }
        inline void tokenize(const char* seq, std::vector<string>& tokens){
            auto q = string(seq);
            std::string processed = preprocess(q);
            boost::trim(processed);
            boost::split_regex(tokens, processed, boost::regex(R"([\s]+)"));
        }
        /**
         * parse a list of functions like ( (mark 3 1 x) ... () )
         * "((MARK 3 3) NOOP)"
         */
        inline std::vector<cnst_term_sptr> parseSeq(const char* seq){
            std::vector<string> tokens;
            tokenize( seq, tokens);
            return parseSeq(tokens);
        }
        inline std::vector<cnst_term_sptr> parseSeq(std::vector<string>& tokens){
            std::vector<cnst_term_sptr> terms;
            auto start = tokens.begin() +1;
            for(auto it = start; it < (tokens.end()-1);){
                transformer->getBalanced(it,tokens.end());
                if( it >= tokens.end() ) throw "GdlParser :: Bad Sequence.";
                if( start != it)
                    terms.push_back(parseFn(start, ++it));
                else{
                    terms.push_back(memCache->getConst(Namer::id(*it)));
                    it++;
                }
                    
                start = it;
            }
            return terms;
        }
        ~GdlParser(){
            delete pool;
            delete transformer;
        }
    };
    
    inline void checkValid(string& s){
        if( !isalnum( s[0] )) 
            throw SyntaxError("[*] GdlParser ::Error:: A literal/term Name should only consist of Alpha Numeric characters.");
    }
} // namespace AresAresAres

#endif