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
    public:
        CallBack(std::atomic<bool>& d,Cache* c)
        :done(d),cache(c)
        {
        } 
        CallBack(const CallBack&) = delete;
        CallBack(const CallBack&&) = delete;
        CallBack& operator=(const CallBack&) = delete;
        CallBack& operator=(const CallBack&&) = delete;

        virtual void operator()(const Substitution&, ushort, bool isLookup=false) = 0;
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
        LiteralCB(const cnst_lit_sptr& lit, unique_cb&& cb_,Cache* c)
        : CallBack(cb_->done,c), lit(lit), cb(std::move(cb_))
        {
        }
        virtual void operator()(const Substitution& sub, ushort suffix, bool isLookup=false);
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
        ClauseCB(Query&& query);
        virtual void operator()(const Substitution& sub, ushort suffix, bool isLookup=false);
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
        ClauseCBOne(std::atomic_bool& done_,Cache* c)
        : CallBack(done_,c)
        {
        }
        virtual void operator()(const Substitution&,ushort, bool isLookup=false){
            done = true;
        }
    };   
} // namespace ares

#endif