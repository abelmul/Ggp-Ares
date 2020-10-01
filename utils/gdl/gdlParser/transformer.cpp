#include "utils/gdl/gdlParser/transformer.hh"
#include "utils/gdl/gdlParser/gdlParser.hh"

namespace ares
{
    void Transformer::getBalanced(vector<string>::iterator& it, const vector<string>::iterator& end){
        short i =0;
        // if( *it != "(" ) throw SyntaxError( "[*] GdlParser ::Error:: Gdl not well formed! Expecting a '('");
        while ( it < end )
        {
            if( *it == "(" ) i++;
            else if( *it == ")" ) i--;
            //Found an expression with balanced parantheses.
            if( i ==  0 )
                break;
            it++;
        };
    }
    /**
     * Applies transformations to the clause body. The transformations are as follows.
     * ~~L to L
     *  A <= A0,..,Ai-1,(W or V),..An      to      A <= A0,..,Ai-1,W,..An and A <= A0,..,Ai-1,V,..An
     *  A <= A0,..,Ai-1,~(W or V),..An     to      A <= A0,..,Ai-1,~W,~V,..An
     *  A <= A0,..,Ai-1,~(W and V),..An    to      A <= A0,..,Ai-1,~W,..An and A <= A <= A0,..,Ai-1,~V,..An
     * Not handling the last case.
     */
    void Transformer::applyTransformations(Clause* c, KnowledgeBase* base,unique_ptr<TokenStream> stream){
        while (stream->current() < stream->end())
            parseFml(c,base, *stream);

        if ( (*(*stream))!= ")" ) throw UnbalancedParentheses( "Gdl :: Error :: Clause needs to be enclosed by parentheses.");
        base->add(c->getHead()->get_name(), c);
    }

    void Transformer::parseFml(Clause* c,KnowledgeBase* base, TokenStream& stream){
        auto& it = stream.current();
        if( *it != "("){
            // A literal without a body
            const Literal* l = parser->parseLiteral(it,stream.end(),true);
            c->getBody().push_back(l);
            ++stream;       //Advances stream position to the next formula.
            return;
        }
        //*it == "(", 
        if(*(it + 1) == "or"){
            /**(or α0 α1 ... αn )
             * Replace A <= A0,..,Ai-1,(W or V),..An    by A <= A0,..,Ai-1,W,..An    and A <= A0,..,Ai-1,V,..An 
             */
            it +=2;
            uint i=0;       //Get αi
            vector<string>::iterator ai = it, next_exp = it;
            vector<TokenStream*> streams;
            while (*ai != ")")
            {
                getBalanced(ref(next_exp), stream.end());
                if( (next_exp+1) >= stream.end() ) throw UnbalancedParentheses("GdlParser:: Error :: Unbalanced parentheses while parsing or");

                if( i != 0 ) streams.push_back( new TokenStream(stream.data(), ai, ai, stream.end()) );
                
                ++next_exp; i++;
                ai = next_exp;
            }
            if( *next_exp != ")") throw SyntaxError("Expecting a parentheses.");
            // if( i < 2 ) throw SyntaxError("Gdlparser :: Error :: Expecting 2 operands to or"); KIF might allow this
            if( i > 1 ){
                for (size_t j = 0; j < streams.size(); j++){
                    auto* s = streams[j];
                    s->setNext(next_exp+1);
                    auto* clone = new Clause(c->head, new ClauseBody(c->body->begin(), c->body->end(),true));
                    parser->pool->post([this,clone,s,base](){
                         this->applyTransformations(clone, base,unique_ptr<TokenStream>(s));
                    });
                }
            }
            stream.setNext(next_exp+1);
            return;
        }
        //*it == "(", 
        else if(*(it+1) == "not"){
            it+=2;
            transformNot(c, ref(it), ref(stream));
        }

        else{
            //Should be a literal

            const Literal* l = parser->parseLiteral(it,stream.end(),true);
            c->getBody().push_back(l);
            ++stream;       //Advances stream position to the next formula.
        }
    }

    //Different cases of not
    void Transformer::transformNot(Clause* c, vector<string>::iterator& it, TokenStream& stream){
        if( *it != "("){
            // A literal without a body
            const Literal* l = parser->parseLiteral(it,stream.end(),false);
            it++;
            if( it >= stream.end() || *it != ")") throw SyntaxError( "GDLParser :: Error :: While parsing not.");
            c->getBody().push_back(l);
            ++stream;       //Advances stream position to the next formula.
            return;
        }
        //*it == "("
        if( *(it+1) == "not" ){
            //~~α == α,  (not (not α)) == α
            //Extract α
            it+=2;
            auto a_end = it;
            getBalanced(ref(a_end), stream.end());
            if ( (a_end+2) >= stream.end() || *(a_end+1) != ")" || *(a_end+2) !=")" ) throw SyntaxError( "GDLParser :: Error :: While parsing not.");
            stream.setNext(a_end+3);
            //Current(it) points to the start α
        }
        //*it == "("
        else if( *(it+1) == "or" ){
            //~( α V β ) === ~α N ~β,   (not (or α β ))
            // extract α
            it+=2;
            auto a_end = it;
            getBalanced(a_end, stream.end());
            if( a_end >= stream.end() ) throw UnbalancedParentheses( "GDlParsor :: error :: Unbalanced parentheses while parsing (not (or");
            // extract β
            auto b = a_end +1;
            auto b_end= b;
            getBalanced(b_end, stream.end());
            if( (b_end+2) >= stream.end() || *(b_end+1) != ")" ||*(b_end +2) != ")") throw UnbalancedParentheses( "Gdl :: Error :: Unblanced parenthesess while parsing clause");
            stream.setNext(b_end+3);
            //create a new stream with (,not, α ,),(, not , β ,), ... stream.end()
            size_t n = distance(it , stream.getNext() ) + distance(stream.getNext() , stream.end()+7);
            vector<string>* data = new vector<string>();
            data->reserve(n);
            data->insert(data->end(), {"(","not"});
            data->insert(data->end(), it, a_end+1);
            data->insert(data->end(), {")","(","not"});
            data->insert(data->end(), b, b_end+1);
            data->push_back(")");
            data->insert(data->end(), stream.getNext(),stream.end()+1);
            stream.replaceData(data);
        }
        else{
            //Should be a literal
            const Literal* l = parser->parseLiteral(it,stream.end(),false);
            c->getBody().push_back(l);
            it++;           //next token
            if( (it >= stream.end())  || ((*it) != ")") ) throw SyntaxError("GdlParser :: Error :: While parsing '(not literal' Expecting ')' before end.");
            ++stream;       //Advances stream position to the next formula.
        }
    }
} // namespace ares
