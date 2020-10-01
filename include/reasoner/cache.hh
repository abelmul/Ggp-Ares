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
#include "utils/utils/iterators.hh"

namespace ares 
{

    
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
        inline AnswerList* addAnswer(const Literal* lit,const Substitution& ans){
            VarSet vset;
            auto soln = (*lit)(ans,vset);
            if( newSolns.push_back((const Literal*)soln) )return this;
            return nullptr;
        }
        /**
         * Mainly used for restarting suspended queries, when an answer has been found.
         */
        template <class T>
        inline void apply(T op){
            ushort j =1-qi;     //Iterate the previous one
            
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
            qi = 1-qi;
        }
        /**
         * This method is implemented just to make testing convinient
         */
        inline std::pair<std::size_t,std::size_t> sizeob(){
            return make_pair(observers[qi].size(),observers[1-qi].size());
        }
        /**
         * This method is implemented just to make testing convinient
         */
        inline std::pair<std::size_t,std::size_t> size(){
            return make_pair(solns.size(),newSolns.size());
        }
        ~AnswerList(){}
        private:    
            typedef std::vector<Query> Observers;

            UniqueVector<const Literal*,LiteralHasher,LiteralHasher> solns;      //Answers found this far
            UniqueVector<const Literal*,LiteralHasher,LiteralHasher> newSolns;   //new Answers                                               
            Observers observers[2]{Observers(),Observers()};                       //Suspended queries
            ushort qi;
            
        friend class Cache;
    };

    struct Next{
        Next():i(0){};
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

            //Restart susspended queries, if any
            for (auto &&ansL : next[j])
                ansL->apply(op);

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
        typedef tbb::concurrent_hash_map<const Literal*,std::unique_ptr<AnswerList>,LiteralHasher> AnsCache;
    private:
        tbb::concurrent_hash_map<const Literal*, std::unique_ptr<AnswerList>, LiteralHasher> ansCache;
        // std::unordered_set<cnst_lit_sptr> failCache;
    public:
        Cache(AnsIterator::Type ansType_=AnsIterator::SEQ)
        :ansType(ansType_)
        {
        }
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
                ac->second.reset(new AnswerList());
                nsolns++;
                return NOT_CACHED;
            }
            //Lookup node. Variant exists as key in cache.
            auto ansIt = ac->second->addObserver(std::move(q));
            nconsumedAns+= ansIt.remaining();
            nlookups++;
            return ansType == AnsIterator::SEQ  ? ansIt : RandomAnsIterator(ansIt);
        }
        /**
         * insert a new answer to the answer list of l.
         */
        inline bool addAns(const Literal* l, const Substitution& ans){
            AnsCache::accessor ac;
            if( !ansCache.find( ac , l) ) throw "Tried to insert solution to a non existent key!";
            if( auto ansl = ac->second->addAnswer(l, ans) ){
                //Found a new answer, register answerlist so that we can restart suspended queries            
                next_.push_back(ansl);
                nnewAns++;
                return true;
            }
            return false;
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
        AnsIterator::Type ansType;
    
    /**
     * Stats
     */
        long nlookups;
        long nnewAns;
        long nsolns;
        long nconsumedAns;
    };
} // namespace Cache

#endif