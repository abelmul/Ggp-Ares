#include "utils/gdl/gdlParser/gdlParser.hh"

namespace Ares
{

    GdlParser* GdlParser::_parser = nullptr;   
    const vector<Term*>* GdlParser::EMPTY_BODY = new vector<Term*>();
    const vector<Literal*>* GdlParser::EMPTY_CBODY = new vector<Literal*>();
    SpinLock GdlParser::slock;
    /**
     * Parse from a gdl file, and populate knowledgebase
     */
    void GdlParser::parseFile(KnowledgeBase* base, string& gdlF){
        this->base = base;
        ifstream f(gdlF);
        string gdl((istreambuf_iterator<char>(f)),
                 istreambuf_iterator<char>());
        string processed = preprocess(gdl);
        parse(processed);
        // pool->join();
    }
    /**
     * Parse a gdl string, and populate knowledgebase
     */
    void GdlParser::parse(KnowledgeBase* base, string& gdl){
        this->base = base;
        string processed = "(" + preprocess(gdl) + ")";
        vector<string>::iterator it,start,end;
        parse(processed);
        // pool->join();
    }

    void GdlParser::parse(string& processed){
        vector<string> tokens;
        boost::trim(processed);
        boost::split_regex(tokens, processed, boost::regex(R"([\s]+)"));

        vector<string>::iterator it, start, end;
        Pstate state = NEW;
        for (it = tokens.begin()+1; it < tokens.end() ;)
        {
            switch (state)
            {
            case NEW:
                if( it >= tokens.end()) throw "[*] GdlParser ::Error:: Gdl not well formed! Expecting a ')' at end.";
                if( (it+1) == tokens.end()){
                    //There should be an ending enclosing ')', ( clauses () () () ... )
                    //it+1 == ')'
                    if( *it != ")" ) throw "[*] GdlParser ::Error:: Gdl not well formed! Expecting a ')' at end.";
                    state = END;
                    it++;  //break out of loop
                    continue;
                }
                if(*it != "(" )  throw "[*] GdlParser ::Error:: Gdl not well formed! Expecting a '('";
                start = it;
                state = BALANCE;
                break;
            case BALANCE:
                getBalanced(it,tokens);
                if( it >= tokens.end() ) throw "[*] GdlParser ::Error:: Gdl not well formed! unbalanced '(' and ')'.";
                end = it;
                if( *(start + 1) != "<" ){
                    //Must be a fact
                    boost::asio::post(*pool, [this,start,end](){
                        //Parse on a separate thread
                        Literal* l =  parseFact(start, end);
                        Clause* c = new Clause(l, GdlParser::EMPTY_CBODY);
                        this->base->add(l->getName(), c);
                    });
                }
                else {
                    Clause* c = new Clause(nullptr, new ClauseBody());
                    boost::asio::post(*pool, [this,start,end, c](){parseRule(c, start, end);});
                }
                it++;
                state = NEW;
                break;
            default:
                break;
            }
        }
        if( state != END ) throw "[*] GdlParser ::Error:: Gdl not well formed!";
    }
    void GdlParser::getBalanced(vector<string>::iterator& it, vector<string>& tokens){
        short i =0;
        if( *it != "(" ) throw "[*] GdlParser ::Error:: Gdl not well formed! Expecting a '('";
        do
        {
            if( *it == "(" ) i++;
            else if( *it == ")" ) i--;
            //Found an expression with balanced parantheses.
            if( i ==  0 )
                break;
            it++;
        } while ( it < tokens.end() );
    }

    /**
     * Remove comments and [\n\r], replace \s\s+ by \s ,
     * replace '\s*(\s*' by '(', replace '\s*)\s*' by ')'
     */
    string GdlParser::preprocess(string& gdl){
        //remove ;[^\n\r]*[\n\r]+
        static boost::regex re_c(R"(;[^\n\r]*([\n\r]+|\Z))");
        static boost::regex re_b(R"((\s*\(\s*)|(\s*\)\s*))");

        string result = boost::regex_replace(gdl,re_c,"");
        return boost::regex_replace(result, re_b,  [](const boost::smatch & match){
            for (auto &&c :  match.str())
                if ( !isspace(c) ) return string(" ") + string(1,c) + string(" ");

            return string();
        });
    }

    /**
     * TODO: YOU DON'T REALLY NEED end!
     */
    Literal* GdlParser::parseFact(vector<string>::iterator start, vector<string>::iterator end ){
        if( *start != "(" or *end != ")") throw "[*] GdlParser ::Error:: A literal should be enclosed by parentheses.";
        
        stack<pair<string, Body*>> bodies;
        checkValid( *(start+1) );       //Next token should be a valid name.
        LitBody* lBody = new LitBody();
        bodies.push(pair<string, Body*>(*(start+1), lBody ));
        auto it = start+2;
        while ( !bodies.empty() and it < end)
        {
            string& token = *it;
            //parse the terms
            if( isalnum(token[0]) ){
                /** 
                 * Must be a constant. get the constant from expression pool.
                 * then add to body.
                 */
                auto& body = bodies.top().second;
                Constant* cnst = exprPool->getConst(token.c_str());
                body->push_back(cnst);
            }
            else if( token[0] == '?' and token.size() > 1 ){
                auto& body = bodies.top().second;
                Variable* var = exprPool->getVar(token.c_str());
                body->push_back(var);
            }
            else if( token == "("){
                //Its a function, Pushing time! ;)
                checkValid(*(it+1));    //Next token should be a valid name.
                bodies.push(pair<string, Body*>( *(it+1), new Body() ));
                it++;
            }
            else if ( token == ")"){
                //Popping time!
                const char* name = bodies.top().first.c_str();
                Body* body = bodies.top().second;
                
                PoolKey key{name, body};
                
                bool exists;

                bodies.pop();
                if( bodies.empty() ){
                    //Its the literals body thats been popped
                    Literal* l = exprPool->getLiteral(key, ref(exists));
                    if( exists ) delete body;
                    return l;
                }
                Function * fn = exprPool->getFn(key,ref(exists));
                if( exists ) delete body;
                bodies.top().second->push_back(fn);
            }
            else 
                throw "GdlParser :: Error :: Unexpected token : " + token + " \nWhile parsing " + *(start +1);

            it++;   //Next token
        }
        //Control should not reach here!
        throw  "GdlParser :: Error :: Unbalanced parantheses while parsing lieral " + *(start +1);   
    }
    void GdlParser::parseRule(Clause* c, vector<string>::iterator start, vector<string>::iterator end ){

    }

    void GdlParser::parseBody(Clause* c, vector<string>::iterator start, vector<string>::iterator end ){

    }
} // namespace Ares
