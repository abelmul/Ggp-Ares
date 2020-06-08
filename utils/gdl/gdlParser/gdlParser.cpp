#include "utils/gdl/gdlParser/gdlParser.hh"

namespace Ares
{

    const std::vector<Term*>* GdlParser::EMPTY_BODY = new std::vector<Term*>();

    void GdlParser::parse(KnowledgeBase* base, std::string gdl){
        this->base = base;
        std::string processed = "(" + preprocess(gdl) + ")";
        std::cout << processed<< std::endl;
        // std::cout << std::count(processed.begin(), processed.end(), '(') << std::endl;
        // std::cout << std::count(processed.begin(), processed.end(), ')') << std::endl;
        std::string::iterator it,start,end;
        // for(it = processed.begin()+1,start = processed.begin()+1; it < processed.end(); it++){  
        // }
        // boost::asio::post(pool,)
    }

    /**
     * Remove comments and [\n\r], replace \s\s+ by \s ,
     * replace '\s*(\s*' by '(', replace '\s*)\s*' by ')'
     */
    std::string GdlParser::preprocess(std::string gdl){
        //remove ;[^\n\r]*[\n\r]+
        static boost::regex re_c(R"(;[^\n\r]*([\n\r]+|\Z))");
        static boost::regex re_n(R"(([\n\r]+)|(\s\s+))");
        static boost::regex re_b(R"((\s*\(\s*)|(\s*\)\s*))");

        std::string result = boost::regex_replace(
                             boost::regex_replace(gdl, re_c, ""),
                             re_n," ");
        return boost::regex_replace(result, re_b,  [](const boost::smatch & match){
            for (auto &&c :  match.str())
                if ( !isspace(c) ) return std::string(1,c);

            return std::string();
        });
    }
    void GdlParser::parse(std::string expr){
        
    }

    Literal* GdlParser::parseFact(std::string expr){

    }

    void GdlParser::parseRule(Clause* c, std::string expr){

    }

    void GdlParser::parseBody(Clause* c, std::string expr){

    }

} // namespace Ares
