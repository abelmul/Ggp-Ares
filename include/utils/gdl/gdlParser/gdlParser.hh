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
    enum Pstate {NEW, BALANCE,END};
    struct TokenStream;
    class GdlParser
    {
        typedef std::function<const structured_term*(std::reference_wrapper<PoolKey>)> Creator;
        friend class Transformer;
    
    /**
     * Methods.
     */
    private:

        /**
         * ctor
         */
        GdlParser(MemCache* mem)
        : pool(ThreadPoolFactroy::get(1)), memCache(mem)
        {
            transformer = Transformer::create(this);
        }
        
        GdlParser(const GdlParser&)=delete;
        GdlParser& operator=(const GdlParser&)=delete;
        GdlParser(const GdlParser&&)=delete;
        GdlParser& operator=(const GdlParser&&)=delete;
        /**
         * Populate base with the rules and facts in tokens. 
         * @param tokens tokenized valid kif/gdl
         * @param base the knowledgebase to be populated
         */
        void parse(vector<string>& tokens,KnowledgeBase* base);
        string preprocess(string& expr);
        /**
         * Parse structured terms -- Functions and literals.
         */
        const structured_term* parseSterm(vector<string>::iterator& start, const vector<string>::iterator& end,const Creator&,bool p=true);
        const structured_term* parseSterm(const char*,const Creator& crtr);
        /**
         * Just a wrapper function.
         */
        inline const Literal* parseLiteral(vector<string>::iterator& start, const vector<string>::iterator& end,bool p=true){
            auto crtr = [=](PoolKey& k){ return memCache->getLiteral(k);};
            auto st = parseSterm(start, end, crtr,p);
            return (const Literal*)st;
        }
        inline const Function* parseFn(vector<string>::iterator& start, const vector<string>::iterator& end){
            auto crtr = [=](PoolKey& k){ return memCache->getFn(k);};
            auto st = parseSterm(start,end,crtr);
            return (const Function*)st;
        }
        /**
         * Parse rules of the form A<-A1,..,An
         */
        void parseRule(Clause* c,KnowledgeBase* base,vector<string>& tokens, vector<string>::iterator start,const vector<string>::iterator end);
        const structured_term* create(stack<pair<string,Body*>>& bodies,const Creator&,bool p=true);
        string removeComments(string gdl);

      

    public:
        //Singleton parser/transformer
        static GdlParser& create(MemCache* mem){
            static GdlParser parser(mem);
            return parser;
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
        inline const Literal* parseQuery(const char * query){
            auto crtr = [=](PoolKey& k){ return memCache->getLiteral(k);};
            auto st = parseSterm(query,crtr);
            return (const Literal*)st;
        }
        /**
         * Just a wrapper function for ease of use.
         * @param fn a string representing a function.
         * @returns a function object.
         */
        inline const Function* parseFn(const char* fn){
            auto crtr = [=](PoolKey& k){ return memCache->getFn(k);};
            auto st = parseSterm(fn,crtr);
            return (const Function*)st;
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
        inline std::vector<const Term*>* parseSeq(const char* seq){
            std::vector<string> tokens;
            tokenize( seq, tokens);
            return parseSeq(tokens);
        }
        inline std::vector<const Term*>* parseSeq(std::vector<string>& tokens){
            auto* terms = new std::vector<const Term*>();
            auto start = tokens.begin() +1;
            for(auto it = start; it < (tokens.end()-1);){
                transformer->getBalanced(it,tokens.end());
                if( it >= tokens.end() ) throw "GdlParser :: Bad Sequence.";
                if( start != it)
                    terms->push_back(parseFn(start, ++it));
                else{
                    terms->push_back(memCache->getConst(Namer::id(*it)));
                    it++;
                }
                    
                start = it;
            }
            return terms;
        }
        ~GdlParser(){
            ThreadPoolFactroy::deallocate(pool);
            log("[~GdlParser]");
        }

    /**
     * Data
     */
    private:
        ThreadPool* pool;
        MemCache* memCache;
        
        //Static members
        static Transformer* transformer;
            
    };
    
    inline void checkValid(string& s){
        if( !isalnum( s[0] ) or (s =="or") or (s == "not")) 
            throw SyntaxError("[*] GdlParser ::Error:: A literal/term Name should only consist of Alpha Numeric characters.");
    }
} // namespace AresAresAres

#endif