#include "utils/gdl/gdlParser/gdlParser.hh"

namespace Ares
{
    /*Define static members*/
    GdlParser* GdlParser::_parser = nullptr;   
    const Body* GdlParser::EMPTY_BODY = new Body();
    ClauseBody* GdlParser::EMPTY_CBODY = new ClauseBody();
    SpinLock GdlParser::slock;
    Transformer* GdlParser::transformer = nullptr;
    /*Define static members*/


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
        pool->join();
    }
    /**
     * Parse a gdl string, and populate knowledgebase
     */
    void GdlParser::parse(KnowledgeBase* base, string& gdl){
        this->base = base;
        string processed = "(" + preprocess(gdl) + ")";
        vector<string>::iterator it,start,end;
        parse(processed);
        pool->join();
    }

    void GdlParser::parse(string& processed){
        boost::trim(processed);
        boost::split_regex(tokens, processed, boost::regex(R"([\s]+)"));
        
        vector<string>::iterator it, start, end;
        Pstate state = NEW;
        for (it = tokens.begin()+1; it < tokens.end();)
        {
            switch (state)
            {
            case NEW:
                if( it >= tokens.end()) throw SyntaxError("[*] GdlParser ::Error:: Gdl not well formed! Expecting a ')' at end.");
                if( (it+1) == tokens.end()){
                    //There should be an ending enclosing ')', ( clauses () () () ... )
                    //it+1 == ')'
                    if( *it != ")" ) throw SyntaxError("[*] GdlParser ::Error:: Gdl not well formed! Expecting a ')' at end.");
                    state = END;
                    it++;  //break out of loop
                    continue;
                }
                if(*it != "(" )  throw SyntaxError("[*] GdlParser ::Error:: Gdl not well formed! Expecting a '('");
                start = it;
                state = BALANCE;
                break;
            case BALANCE:
                transformer->getBalanced(it,tokens.end());
                if( it >= tokens.end() ) throw SyntaxError( "[*] GdlParser ::Error:: Gdl not well formed! unbalanced '(' and ')'.");
                end = it;
                if( *(start + 1) != "<=" ){
                    //Must be a fact
                    boost::asio::post(*pool, [this,start,end](){
                        //Parse on a separate thread
                        vector<string>::iterator s = start;
                        try
                        {
                            const Literal* l =  parseLiteral(s, end+1,true);
                            Clause* c = new Clause(l, GdlParser::EMPTY_CBODY);
                            this->base->add(l->getName(), c);
                        }
                        catch(std::runtime_error& e)
                        {
                            std::cout << "In catch "  << e.what()<< '\n';
                            fflush(NULL);
                        }
                    });
                }
                else { 
                    Clause* c = new Clause(nullptr, new ClauseBody());
                    boost::asio::post(*pool, [this,start,end, c](){
                    try
                    {
                        parseRule(c, start, end);
                    }
                    catch(const std::runtime_error& e)
                    {
                        std::cerr << e.what() << '\n';
                    }
                    });
                }
                it++;
                state = NEW;
                break;
            default:
                break;
            }
        }
        if( state != END ) throw SyntaxError( "[*] GdlParser ::Error:: Gdl not well formed!");
    }
    /**
     * Remove comments and [\n\r], replace \s\s+ by \s ,
     * replace '\s*(\s*' by '(', replace '\s*)\s*' by ')'
     */
    string GdlParser::preprocess(string& gdl){
        //remove ;[^\n\r]*[\n\r]+
        static boost::regex re_c(R"(;[^\n\r]*([\n\r]+|\Z))");
        static boost::regex re_b(R"((\s*\(\s*)|(\s*\)\s*))");

        string cmtRemoved = boost::regex_replace(gdl,re_c,"");
        string result =  boost::regex_replace(cmtRemoved, re_b,  [](const boost::smatch & match){
            for (auto &&c :  match.str())
                if ( !isspace(c) ) return string(" ") + string(1,c) + string(" ");

            return string();
        });
        boost::to_lower(result);
        return result;
    }

    const Literal* GdlParser::parseLiteral(vector<string>::iterator& start, const vector<string>::iterator& end ,bool p){
        stack<pair<string, Body*>> bodies;
        if( *start != "(" ){
            //Its a literal without  a body like 'terminal'
            if( start >= end ) throw SyntaxError( "GDLParser :: Error :: Literal start > end");
            checkValid(*start); //valid name
            bodies.push(pair<string, Body*>(*start, new Body() ));
            const Literal* l = _create(bodies,p);
            return l;
        }
        checkValid( *(++start) );       //Next token should be a valid name.
        auto& name = *start++;
        bodies.push(pair<string, Body*>(name, new Body() ));
        auto& it = start;
        const Literal* l;

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
                const Constant* cnst = exprPool->getConst(token.c_str());
                body->push_back(cnst);
            }
            else if( token[0] == '?' and token.size() > 1 ){
                auto& body = bodies.top().second;
                const Variable* var = exprPool->getVar(token.c_str());
                body->push_back(var);
            }
            else if( token == "("){
                //Its a function, Pushing time! ;)
                checkValid(*++it);    //Next token should be a valid name.
                bodies.push(pair<string, Body*>( string(*it), new Body() ));
            }
            else if ( token == ")"){
                //Pop an element and check if we are done.
                //a well formed expression should eventually get into the next if.
                if( (l=_create(ref(bodies),p)) )   
                    return l;
            }
            else 
                throw SyntaxError("GdlParser :: Error :: Unexpected token : " + token + " \nWhile parsing " + name);

            it++;   //Next token
        }
        //Control should not reach here!
        throw  UnbalancedParentheses("GdlParser :: Error :: Unbalanced parantheses while parsing literal " + name);
    }
    const Literal* GdlParser::_create(stack<pair<string,Body*>>& bodies,bool p) {
        auto s = bodies.top().first;
        const char* name = s.c_str();
        Body* body = bodies.top().second;
        
        PoolKey key{name, body,true};
        
        bool exists;

        bodies.pop();
        if( bodies.empty() ){       //Balanced parentheses
            //Its the literals body thats been popped
            key.p = p;
            const Literal* l = exprPool->getLiteral(key, ref(exists));
            if( exists ) delete body;
            return l;
        }
        //You can do further check to ensure Function and literal names are distinct! if necessary.
        const Function * fn = exprPool->getFn(key,ref(exists));
        if( exists ) delete body;
        bodies.top().second->push_back(fn);
        return nullptr;
    }
    void GdlParser::parseRule(Clause* c, vector<string>::iterator start,const vector<string>::iterator end ){
        //*start == "(" , *end == ")" and *(start+1) == "<=" checked in parse(...) above
        start += 2;
        //Get the head
        const Literal* head = parseLiteral(ref(start), end, true);
        c->setHead(head);
        start++;    //advance to the next token
        if( start >= end ) throw SyntaxError( "GdlParser :: Error :: Empty Rule Body");
        //populate the body by applying transformations as necessary
        TokenStream* stream = new TokenStream(tokens, start, start, end);
        transformer->applyTransformations(c, unique_ptr<TokenStream>(stream));
    }
} // namespace Ares
