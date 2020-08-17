#ifndef CALLBACKS_HH
#define CALLBACKS_HH
#include "threadPool.hh"
#include "utils/gdl/gdl.hh"
#include <atomic>

namespace ares
{
    class Cache;
    class CallBack
    {
        typedef std::function<void(const Substitution&)> CB_t;
    public:
        CallBack(std::atomic<bool>& d,Cache* c)
        :done(d),cache(c)
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
        LiteralCB(const cnst_lit_sptr& lit, unique_cb&& cb,Cache* cache)
        : CallBack(cb->done,cache), lit(lit), cb(std::move(cb))
        {
        }
        virtual void operator()(const Substitution& ans);
        virtual ~LiteralCB(){}
    
    /**
     * Data
     */
    private:
        const cnst_lit_sptr lit;
        unique_cb cb;
        
    };
    class Prover;
    class ClauseCB : public CallBack
    {
    
    /**
     * Methods
     */
        typedef std::unique_ptr<Clause> unique_clause;
    public:
        ClauseCB(Query&& query,Cache* cache);
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
    class ClauseCBOne : public CallBack
    {
    public:
        ClauseCBOne(std::atomic_bool& done,Cache* c)
        : CallBack(done,c)
        {
        }
        virtual void operator()(const Substitution&){
            done = true;
        }
    };   
} // namespace ares

#endif