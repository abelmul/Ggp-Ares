#ifndef GDL_HH
#define GDL_HH

#include "term.hh"
#include "variable.hh"
#include "constant.hh"
#include "function.hh"
#include "atom.hh"
#include "clause.hh"
#include "or.hh"
#include "not.hh"

namespace ares{
    class CallBack;
    class State;
    class ThreadPool;
    class Cache;

    struct Query{
        typedef std::unique_ptr<Clause> unique_clause;
        typedef std::shared_ptr<CallBack> shared_callback;

        Query(unique_clause& g,const shared_callback& cb,const State* s,Cache* c,ushort suff,bool rand)
        :context(s), cb(cb),goal( std::move( g)),
         cache(c),suffix(suff),id(nextId++),random(rand), ptr(0)
        {
        }

        Query(const Query& q)
        :context(q.context), cb(q.cb), goal( std::move( *(unique_clause*)&q.goal)),
        cache(q.cache),suffix(q.suffix),id(q.id), random(q.random),ptr(q.ptr)
        {
        }
        inline static void resetid(){nextId = 0;}
        Clause* operator->()const{ return goal.get();}
        Clause& operator* ()const{ return *goal;}
        const State* context;
        std::shared_ptr<CallBack> cb;
        unique_clause goal;
        Cache* cache;
        const ushort suffix;
        const uint id;      //The id is just to make some tests easier
        bool random;

        private:
            uint ptr;
            static std::atomic<int> nextId;

        friend class AnswerList;
    };
}
#endif