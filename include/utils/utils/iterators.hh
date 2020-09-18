#ifndef ITERATORS_HH
#define ITERATORS_HH
#include <vector>
#include <unordered_map>
#include <unordered_set>
#include <random>
#include <ctype.h>
#include "utils/memory/memoryPool.hh"
#include "utils/utils/hashing.hh"
namespace ares
{

    /**
     * Inorder to restart a query we need to know how much of 
     * the answer we have consumed so far.
     * This class encapsulate that logic. 
     */
    struct AnsIterator{
        enum Type{SEQ,RAND};
        typedef std::unique_ptr<Clause> unique_clause;
        typedef std::vector<cnst_lit_sptr>::const_iterator iterator;
        typedef UniqueVector<cnst_lit_sptr,LiteralHasher,LiteralHasher> container;

        AnsIterator(const container* c,const uint curr,Clause* nxtc )
        :ans(c),current(curr),i(0)
        {nxt.reset(nxtc);}
        
        /**
         * Copy Constructor
         */
        AnsIterator(const AnsIterator& other):
        nxt( std::move(*(unique_clause*)&other.nxt)),ans(other.ans),current(other.current),i(other.i)
        {
        }

        virtual operator bool()const{ return ans and ( (current + i) < ans->size()); }
        virtual bool null() { return ans==nullptr;}
        virtual AnsIterator& operator++(){ i++; return *this;}

        virtual const cnst_lit_sptr& operator*()const{ return (*ans)[current + i];}

        virtual std::size_t remaining() { return ans->size() - (current + i);}
        virtual ~AnsIterator(){}

        bool operator!=(const AnsIterator&) = delete;
        bool operator==(const AnsIterator&) = delete;
        /**
         * DATA.
         */
        std::unique_ptr<Clause> nxt;
        protected:
            const container* ans;
            const uint current;
        
        private:
            uint i;
        
        friend class Cache;
        friend class RandomAnsIterator;
    };
   
    /**
     * Experimental! Randomly iterate over the elements.
     */
    struct RandomAnsIterator : public AnsIterator{

        RandomAnsIterator(const AnsIterator& ansi)
        :AnsIterator(ansi)
        {
            if( (not ans) or not (current < ans->size()) ) return;
            gen = std::mt19937(time(0));
            distr = std::uniform_int_distribution<int>(current, ans->size()-1);
            indx = distr(gen);
        }
        
        /**
         * True if there are elements not consumed yet.
         */
        virtual operator bool()const{  return ans and ( seen.size()  < (ans->size() - current)); }
        
        /**
         * 
         */
        virtual AnsIterator& operator++(){ 
            seen.insert(indx);
            if( not (*this) )
                return *this;

            int nIndx = distr(gen);
            while( seen.find(nIndx) != seen.end() )
                nIndx = distr(gen);

            indx = nIndx;
            return *this;
        }
 
        virtual const cnst_lit_sptr& operator*()const{ return (*ans)[indx];}

        virtual std::size_t remaining() { return ans->size() - seen.size();}

        virtual ~RandomAnsIterator(){}
        
        private:
            std::unordered_set<uint> seen;
            uint indx;
            std::mt19937 gen; 
            std::uniform_int_distribution<> distr;

    };
    template <class T>
    class UIterator
    {
    public:
        UIterator(const std::vector<T>& ct):indx(0),cntr(ct){}

        /**
         * True if there are elements not consumed yet.
         */
        virtual operator bool()const{  return ( indx  < cntr.size() ); }

        /**
         * 
         */
        virtual UIterator<T>& operator++(){ ++indx; return *this;}

        virtual const T& operator*()const{ return cntr.at(indx);}

        virtual ~UIterator() {}

    /**
     * DATA
     */
    protected:
        uint indx;
        const std::vector<T>& cntr;
    };
    /**
     * An Iterator For UniqueVector.
     */
    template <class T>
    class RandIterator : public UIterator<T>
    {
    public:
        RandIterator(const std::vector<T>& ct):UIterator<T>(ct){
            if( ct.size() == 0 ) return;
            gen = std::mt19937(time(0));
            distr = std::uniform_int_distribution<int>(0, ct.size()-1);
            this->indx = distr(this->gen);
        }

        /**
         * True if there are elements not consumed yet.
         */
        virtual operator bool()const{  return ( seen.size()  < this->cntr.size() ); }

        /**
         * 
         */
        virtual UIterator<T>& operator++(){ 
            seen.insert(this->indx);
            if( not (*this) )
                return *this;

            int nIndx = distr(this->gen);
            while( seen.find(nIndx) != seen.end() )
                nIndx = distr(this->gen);

            this->indx = nIndx;
            return *this;
        }

        virtual const T& operator*()const{ return this->cntr.at(this->indx);}

        virtual ~RandIterator() {}

    /**
     * DATA
     */
    private:
        std::unordered_set<uint> seen;
        std::mt19937 gen; 

        std::uniform_int_distribution<> distr;
    };
    /**
     * Iterators for strategies to iterate over actions and states.
     */
    typedef cnst_term_sptr moves_sptr;
    typedef std::vector<moves_sptr> Moves;
    class Reasoner;
    class State;
    /**
     * An iterator to iterate over actions.
     */
    class ActionIterator
    {
    private:
        uint i;
        const State* state;
        std::vector<std::unique_ptr<Moves>>* actions;
        Reasoner* reasoner;

    public:
        typedef Moves Action;
        typedef std::unique_ptr<const Action> UniqueAction;
        ActionIterator(const State* s,Reasoner* r):i(0),state(s),actions(nullptr),reasoner(r){}

        ActionIterator(const ActionIterator&)=delete;
        ActionIterator& operator=(const ActionIterator&)=delete;
        ActionIterator(const ActionIterator&&)=delete;
        ActionIterator& operator=(const ActionIterator&&)=delete;

        void init();
        inline virtual operator bool(){
            if( not actions ) init();
            return i < actions->size();
        }

        inline virtual void operator++(){ ++i; }
        /**
         * This functions releases ownership of the current Action*.
         * Thus calling it twice is invalid.
         * @returns the current Action.
         */
        inline virtual Action* operator*(){
            if( not actions ) init();
            return (*actions)[i].release();
        }
        virtual ~ActionIterator() {
            if( actions ) delete actions;
        }
    };
} // namespace ares

#endif