#ifndef CLAUSE_HH
#define CLAUSE_HH

#include "utils/gdl/literal.hh"
#include "utils/gdl/function.hh"
#include "reasoner/substitution.hh"
#include "reasoner/suffixRenamer.hh"
#include <stack>
#include<string>
namespace ares
{
    /**
     * Apparently std::vector is faster than std::list even for situations 
     * involving lots of insertions and deletions!
     */ 

    class Clause
    {
        friend class Transformer;
        friend class ClauseHasher;
        friend class visualizer;
        
    private:
        cnst_lit_sptr head = nullptr;
        ClauseBody* _body = nullptr;
        int i=0;
        ClauseBody& body;
        Substitution* theta = nullptr;
        ClauseBody& getBody(){return body;}

    public:
        // clause_body_mem_pool
        /**
         * Protect against accidental copy assignment, pass by value,...
         */
        Clause(const Clause& c) = delete;
        Clause(const Clause&&) = delete;
        Clause& operator =(const Clause&) = delete;
        Clause& operator =(const Clause&&) = delete;
        

        /**
         * A clause has this kind of form:
         * A <- A0 and ... and An
         * where A0...An are literals(the body), and A is the head
         */
        Clause(cnst_lit_sptr head,ClauseBody* _b)
        :head(head),_body(_b),body(std::ref(*_body)),theta(nullptr)
        {
            i=0;
        };

        /**
         * Override operator new and delete, for custom memory mgmt.
         */
        void* operator new(std::size_t);
        void  operator delete(void*);
        ~Clause(){
            if (theta) delete theta;
            if( _body ) delete _body;
            head = nullptr;
            theta = nullptr;
            _body = nullptr;
        }


        static bool EMPTY_CLAUSE(const Clause& c) { return (c.body.size() == 0  and (not c.head) ); }

        Clause* clone() const{
            return new Clause(head, new ClauseBody(body.begin(), body.end()));
        }
        /**
         * @brief Create a new renamed clause, used in a resolution step while resolving
         * a goal.
         * @param renamedBody.size() >= body.size()
         */ 
        void renameBody(ClauseBody& renamedBody, SuffixRenamer& vr) const {
            
            // renamed->body.resize()
            auto vset = VarSet();
            for (uint i =0;i < body.size() ; i++){
                auto& l = body[i];
                const cnst_term_sptr& lr = (*l)(vr, vset);       //Apply renaming
                renamedBody[i] = *((cnst_lit_sptr*)&lr);
            }
            return;
        }
        /**
         * @brief insert the elements from 
         * @param c.body[ @param offset ] to c.body.end() to 
         * this->body[ @param pos ] to this->body.end()
         * this->body should have space for c.body.size() - offset elements
         */
        void insert(std::size_t pos, const Clause& c, std::size_t offset = 1){
            //Experiment with both inserting at the back and the front
            for (size_t i = offset; i < c.body.size(); i++)
            {
                auto j = pos + i - offset;
                this->body[j] = c.body[i];
            }
            // this->body.insert(body.begin()+pos, c.body.begin()+1, c.body.end());
        }
        std::size_t size()const { return body.size(); }

        void setHead(cnst_lit_sptr h){head = h;}
        const cnst_lit_sptr& getHead() const { 
            if ( head )return head; 
            return Term::null_literal_sptr;
        }
        // void setBody(ClauseBody* b){ if(!_body) _body = b;}

        Substitution& getSubstitution() const { return *theta;}
        void setSubstitution(Substitution* t){ theta = t;}

        const cnst_lit_sptr& front() const { return body[0];}

        void pop_front() { body.pop_front();}

        void delayFront(){
            body.front_to_back();
        }
        
        /**
         * TODO: Implement hashing so that only order of variables matter not their name.
         */
        std::size_t hash()const{ return 0;}

        std::string to_string()const{
            std::string s("");
            if( body.size() > 0) s.append("(");
            if( head ) s.append( head->to_string() + " ");
            if( body.size() > 0) s.append(" <= ");

            for (auto &l : body)
                s.append("\n" + l->to_string());

            if( body.size() > 0) s.append(")");
        if( theta and (not theta->isEmpty()) ) 
            s.append("\nSubstitution :\n" + theta->to_string());
        return s;
        }
        friend std::ostream & operator << (std::ostream &out, const Clause &c){
            out << c.to_string();
            return out;
        }
    };
    
} // namespace ares

#endif