#ifndef GDL_HH
#define GDL_HH

#include "utils/gdl/term.hh"
#include "utils/gdl/variable.hh"
#include "utils/gdl/constant.hh"
#include "utils/gdl/function.hh"
#include "utils/gdl/literal.hh"
#include "utils/gdl/clause.hh"

namespace ares{
    class CallBack;
    class State;
    struct Query{
        typedef std::unique_ptr<Clause> unique_clause;
        typedef std::shared_ptr<CallBack> shared_callback;

        Query(unique_clause& g,shared_callback& cb,const State* s)
        :context(s), cb(cb),goal( std::move( g)), ptr(0)
        {
        }

        Query(const Query& q)
        :context(q.context), cb(q.cb), goal( std::move( *(unique_clause*)&q.goal)), ptr(q.ptr)
        {
        }
        Clause* operator->()const{ return goal.get();}
        Clause& operator* ()const{ return *goal;}
        const State* context;
        std::shared_ptr<CallBack> cb;
        unique_clause goal;

        private:
            uint ptr;
            static std::atomic<int> nextId;

        friend class AnswerList;
    };
}
#endif