#ifndef GDL_PARSER_HH
#define GDL_PARSER_HH
#include "utils/gdl/term.hh"
#include "utils/gdl/variable.hh"
#include "utils/gdl/constant.hh"
#include "utils/gdl/function.hh"
#include "utils/gdl/literal.hh"
#include "utils/game/state.hh"
#include "utils/gdl/clause.hh"
#include <iostream>
#include <fstream>
#include <string>
#include <boost/regex.hpp>
#include <boost/asio.hpp>

namespace Ares
{
    typedef boost::asio::thread_pool thread_pool;
    class GdlParser
    {
    private:
        std::string preprocess(std::string expr);
        Literal* parseFact(std::string expr);
        void parseRule(Clause* c, std::string expr);
        void parseBody(Clause* c, std::string expr);
        void parse(std::string expr);
        
        thread_pool* pool;
        KnowledgeBase* base;
        const static std::vector<Term*>* EMPTY_BODY;

    public:
        GdlParser(uint nThreads):pool(new thread_pool(nThreads)){}
        void parse(KnowledgeBase* base, std::string gdl);
        ~GdlParser(){}
    };
    
} // namespace Ares

#endif