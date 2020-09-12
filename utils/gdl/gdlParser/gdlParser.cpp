#include "utils/gdl/gdlParser/gdlParser.hh"

namespace ares
{
    /*Define static members*/
    Transformer* GdlParser::transformer = nullptr;
    /*Define static members*/
    
    cnst_st_term_sptr GdlParser::parseSterm(const char* st,const Creator& crtr){
        std::vector<string> tokens;
        tokenize(st,tokens);
        tokens.push_back(")");
        auto start = tokens.begin();
        return parseSterm(start,tokens.end()+1,crtr,true);
    }
    /**
     * Parse from a gdl file, and populate knowledgebase
     */
    void GdlParser::parse(KnowledgeBase* base, string& gdlF){
        pool->restart();
        ifstream f(gdlF);
        string gdl((istreambuf_iterator<char>(f)),
                 istreambuf_iterator<char>());
        gdl = "(" + gdl +")";
        string cmtRemoved = removeComments(gdl);
        std::vector<string> tokens;
        tokenize(cmtRemoved.c_str(),tokens);
        parse(tokens,base);
        pool->wait();
    }
    /**
     * Parse a gdl string, and populate knowledgebase
     */
    void GdlParser::parse(KnowledgeBase* base, vector<string>& tokens){
        pool->restart();
        parse(tokens,base);
        pool->wait();
    }

    void GdlParser::parse(vector<string>& tokens,KnowledgeBase* base){
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
                    pool->post([this,start,end,base](){
                        //Parse on a separate thread
                        vector<string>::iterator s = start;
                        cnst_lit_sptr l =  parseLiteral(s, end+1,true);
                        Clause* c = new Clause(l, new ClauseBody(0));
                        base->add(l->get_name(), c);
                    });
                }
                else { 
                    Clause* c = new Clause(nullptr, new ClauseBody());
                    pool->post([this,start,end, c,&tokens,base](){ parseRule(c, base,tokens,start, end);});
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
    string GdlParser::removeComments(string gdl){
        static boost::regex re_c(R"(;[^\n\r]*([\n\r]+|\Z))");
        return boost::regex_replace(gdl,re_c,"");
    }
    /**
     * Remove comments and [\n\r], replace \s\s+ by \s ,
     * replace '\s*(\s*' by '(', replace '\s*)\s*' by ')'
     */
    string GdlParser::preprocess(string& gdl){
        //remove ;[^\n\r]*[\n\r]+
        static boost::regex re_b(R"((\s*\(\s*)|(\s*\)\s*))");

        auto result = boost::regex_replace(gdl, re_b,  [](const boost::smatch & match){
            for (auto &&c :  match.str())
                if ( !isspace(c) ) return string(" ") + string(1,c) + string(" ");

            return string();
        });
        boost::to_lower(result);
        return result;
    }

    cnst_st_term_sptr GdlParser::parseSterm(vector<string>::iterator& start, const vector<string>::iterator& end ,const Creator& crtr,bool p){
        stack<pair<string, Body*>> bodies;
        if( *start != "(" ){
            //Its a literal without  a body like 'terminal'
            if( start >= end ) throw SyntaxError( "GDLParser :: Error :: Literal start > end");
            checkValid(*start); //valid name
            bodies.push(pair<string, Body*>(*start, new Body(0)));
            return _create(bodies,crtr,p);
        }
        checkValid( *(++start) );       //Next token should be a valid name.
        auto& name = *start++;
        bodies.push(pair<string, Body*>(name, new Body() ));
        auto& it = start;
        cnst_st_term_sptr l;

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
                cnst_const_sptr cnst = memCache->getConst(Namer::registerName(token));
                body->push_back(cnst);
            }
            else if( token[0] == '?' and token.size() > 1 ){
                auto& body = bodies.top().second;
                cnst_var_sptr var = memCache->getVar(Namer::registerVname(token));
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
                if( (l=_create(ref(bodies),crtr,p)) )
                    return l;
            }
            else 
                throw SyntaxError("GdlParser :: Error :: Unexpected token : " + token + " \nWhile parsing " + name);

            it++;   //Next token
        }
        //Control should not reach here!
        throw  UnbalancedParentheses("GdlParser :: Error :: Unbalanced parantheses while parsing literal " + name);
    }
    cnst_st_term_sptr GdlParser::_create(stack<pair<string,Body*>>& bodies,const Creator& crtr,bool p) {
        auto name = bodies.top().first;
        Body* body = bodies.top().second;
        
        PoolKey key{Namer::registerName(name), body,true,nullptr};
        
        bodies.pop();
        if( bodies.empty() ){       //Balanced parentheses
            //Its the root terms body thats been popped
            key.p = p;
            return crtr(key);
        }
        //You can do further check to ensure Function and literal names are distinct! if necessary.
        cnst_fn_sptr fn = memCache->getFn(key);
        bodies.top().second->push_back(fn);
        return nullptr;
    }
    void GdlParser::parseRule(Clause* c, KnowledgeBase* base,vector<string>& tokens,vector<string>::iterator start,const vector<string>::iterator end ){
        //*start == "(" , *end == ")" and *(start+1) == "<=" checked in parse(...) above
        start += 2;
        //Get the head
        cnst_lit_sptr head = parseLiteral(ref(start), end, true);
        c->setHead(head);
        start++;    //advance to the next token
        if( start >= end ) throw SyntaxError( "GdlParser :: Error :: Empty Rule Body");
        //populate the body by applying transformations as necessary
        TokenStream* stream = new TokenStream(tokens, start, start, end);
        transformer->applyTransformations(c, base,unique_ptr<TokenStream>(stream));
    }
} // namespace ares
