#ifndef REASONER_CACHE_HH
#define REASONER_CACHE_HH
#include <vector>
#include <unordered_map>
#include <unordered_set>
#include "utils/memory/memoryPool.hh"
#include "substitution.hh"
#include "utils/game/match.hh"
#include "utils/utils/exceptions.hh"
#include "utils/gdl/gdlParser/gdlParser.hh"
#include "utils/memory/memCache.hh"
#include "reasoner/unifier.hh"
#include "utils/utils/cfg.hh"
#include "utils/threading/threading.hh"
#include "reasoner/suffixRenamer.hh"

namespace ares 
{
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
    /**
     * Inorder to restart a query we need to know how much of 
     * the answer we have consumed so far.
     * This class encapsulate that logic. 
     */
    struct AnsIterator{
        typedef std::vector<cnst_lit_sptr>::const_iterator iterator;
        typedef UniqueVector<cnst_lit_sptr,LiteralHasher,LiteralHasher> container;

        AnsIterator(const container* c,const uint curr,Clause* nxt )
        :ans(c), current(curr),nxt(nxt)
        {}

        inline iterator begin()const{ 
            if(not ans) throw "Tried to call begin on empty iterator.";
            return ans->begin() + current; 
        }
        inline iterator end()const{ 
            if(not ans) throw "Tried to call end on empty iterator."; 
            return ans->end();
        }
        inline bool operator==(const AnsIterator& other)const{
            return (ans == other.ans) and ( current == other.current) and ( nxt == other.nxt);
        }

        ~AnsIterator(){ if( nxt ) delete nxt;}
        Clause* nxt;
        private:
            const container* ans;
            const uint current;
    };

    struct AnswerList
    {
        AnswerList():qi(0){
            newSolns.share(solns);
        }
        AnswerList(const AnswerList&)=delete;
        AnswerList(const AnswerList&&)=delete;
        AnswerList& operator=(const AnswerList&)=delete;
        AnswerList& operator=(const AnswerList&&)=delete;
        
        inline AnsIterator addObserver(Query&& q){
            AnsIterator it(&solns,q.ptr,q->next());
            q.ptr = solns.size();
            observers[qi].push_back(std::move(q));
            return it;
        }
        inline AnswerList* addAnswer(const cnst_lit_sptr& lit,const Substitution& ans){
            VarSet vset;
            auto soln = (*lit)(ans,vset);
            if( newSolns.push_back(*(cnst_lit_sptr*)&soln) )return this;
            return nullptr;
        }
        /**
         * Mainly used for restarting suspended queries, when an answer has been found.
         */
        template <class T>
        inline void apply(T op){
            ushort j =qi;
            qi = 1-qi;
            for (auto &&query : observers[j])
                op(query);
            
            observers[j].clear();
        }
        /**
         * Include the new solutions
         */
        inline void next() {
            solns.copy_elements(newSolns);
            newSolns.clear();
        }
        ~AnswerList(){}
        private:    
            typedef std::vector<Query> Observers;

            UniqueVector<cnst_lit_sptr,LiteralHasher,LiteralHasher> solns;      //Answers found this far
            UniqueVector<cnst_lit_sptr,LiteralHasher,LiteralHasher> newSolns;   //new Answers                                               
            Observers observers[2]{Observers(),Observers()};                       //Suspended queries
            ushort qi;
            
        friend class Cache;
    };

    struct Next{
        Next() = default;
        inline void push_back(AnswerList* ansl){
            std::lock_guard<SpinLock> lk(slck);
            next[i].push_back(ansl);
        }
        template <class T>
        inline void apply(T op){
            ushort j;
            {
                std::lock_guard<SpinLock> lk(slck);
                j = i;
                i = 1-i;
            }
            //Include the new solutions
            for (auto &&ansL : next[j])
                ansL->next();

            for (auto &&ansL : next[j])
                op(*ansL);

            next[j].clear();
        }
        inline std::size_t size(){ return next[i].size(); }
        inline bool empty(){ return size() == 0 ; }
        private:
            UniqueVector<AnswerList*> next[2]{UniqueVector<AnswerList*>(),UniqueVector<AnswerList*>()};
            SpinLock slck;
            ushort i;
    };
    /**
     * Cache answers to some atoms. variant atoms are hashed the same
     * and share answers.
     */
    class Cache
    {
        typedef tbb::concurrent_hash_map<cnst_lit_sptr,AnswerList*,LiteralHasher> AnsCache;
    private:
        tbb::concurrent_hash_map<cnst_lit_sptr, AnswerList*, LiteralHasher> ansCache;
        // std::unordered_set<cnst_lit_sptr> failCache;
    public:
        Cache(){}
        /**
         * Get the indexed solutions list if this query has been cached before.
         * else create an empty solutions list
         * @param q the query to check cache status
         * @returns NOT_CACHED if this query hasn't been seen before else
         * it returns the indexed solutions list iterator.
         */
        inline AnsIterator operator[](Query& q){
            AnsCache::accessor ac;
            if (ansCache.insert(ac, q->front())){
                //Solution node. Has been successfully inserted.
                ac->second = new AnswerList();
                return NOT_CACHED;
            }
            else
                //Lookup node. Variant exists as key in cache.
                return ac->second->addObserver(std::move(q));
        }
        /**
         * insert a new answer to the answer list of l.
         */
        inline void addAns(const cnst_lit_sptr& l, const Substitution& ans){
            AnsCache::accessor ac;
            if( !ansCache.find( ac , l) ) throw "Tried to insert solution to a non existent key!";
            if( auto ansl = ac->second->addAnswer(ac->first, ans) )
                //Found a new answer, register answerlist so that we can restart suspended queries            
                next_.push_back(ansl);
        }
        /**
         * Has any new solutions been found after the previous stage.
         */
        inline bool hasChanged(){ return !next_.empty();}
        /**
         * Start the next stage by restarting suspended queries.
         */
        template <class T>
        inline void next(T op){
            next_.apply(op);
        }
        ~Cache() {}
    
    /**
     * Data
     */
    public:
        static AnsIterator NOT_CACHED;
        Next next_;
    };
} // namespace Cache

#endif