#ifndef CALLBACKS_HH
#define CALLBACKS_HH
#include "reasoner/cache.hh"
#include "threadPool.hh"
#include "utils/gdl/clause.hh"
#include <atomic>

namespace ares
{
    class Cache;
    class CallBack
    {
    public:
        CallBack(std::atomic<bool>& d,Cache* c)
        :done(d), cache(c)
        {
        } 

        CallBack(const CallBack&) = delete;
        CallBack(const CallBack&&) = delete;
        CallBack& operator=(const CallBack&) = delete;
        CallBack& operator=(const CallBack&&) = delete;

        virtual void operator()(const Substitution&)=0;
        virtual ~CallBack() {}
    /**
     * Data
     */
    public:
        std::atomic<bool>& done;
    
    protected:
        Cache* cache;
    };
    
    class LiteralCB : public CallBack
    {
        typedef std::unique_ptr<CallBack> unique_cb;
    /**
     * Methods
     */
    public:
        LiteralCB(const cnst_lit_sptr& lit, bool c,unique_cb&& cb,Cache* cache)
        : CallBack(cb->done,cache), lit(lit), cb(std::move(cb)), contextual(c)
        {
        }
        virtual void operator()(const Substitution& ans);
        virtual ~LiteralCB();
    
    /**
     * Data
     */
    private:
        const cnst_lit_sptr lit;
        unique_cb cb;
        bool contextual;
    };

    //  Query(const Query& q)
    //     :goal( std::move(*(unique_clause*)&q.goal)),context(q.context),cb(q.cb),pool(q.pool),renamer(q.renamer),oneAns(q.oneAns),done(q.done)
    //     {

    //     }
    class Prover;
    class ClauseCB : public CallBack
    {
    
    /**
     * Methods
     */
        typedef std::unique_ptr<Clause> unique_clause;
    public:
        ClauseCB(Query&& query,Cache* cache)
        : CallBack(query.cb->done,cache), nxt(query)
        {
        }

        ClauseCB(Query&& query,std::atomic_bool& done,Cache* cache)
        : CallBack(done,cache), nxt(query)
        {
        }
        virtual void operator()(const Substitution& ans);
        virtual ~ClauseCB() {}

    /**
     * Data
     */
    protected:
        Query nxt;

    public:
        static Prover* prover;

    };

    /**
     * A callback that stops further computation as soon as 1 answer is found.
     */
    class ClauseCBOne : public ClauseCB
    {
    public:
        ClauseCBOne(Query&& query,Cache* c)
        : ClauseCB(std::move(query),_done,c),_done(false)
        {
        }
        virtual void operator()(const Substitution&){
            done = true;
            ClauseCB::operator()(Substitution());
        }
    /**
     * Data
     */
    private:
        std::atomic_bool _done;
    };   
} // namespace ares

#endif