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
        ClauseBody* body = nullptr;
        Substitution* theta = nullptr;
        ClauseBody& getBody(){return *body;}

    public:
        // clausebody_mem_pool
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
        :head(head),body(_b),theta(nullptr)
        {
        };

        Clause(cnst_lit_sptr head,ClauseBody* _b,Substitution* t)
        :head(head),body(_b),theta(t)
        {
        };


        /**
         * Override operator new and delete, for custom memory mgmt.
         */
        void* operator new(std::size_t);
        void  operator delete(void*);
        ~Clause(){
            if (theta) delete theta;
            if( body ) delete body;
            head = nullptr;
            theta = nullptr;
            body = nullptr;
        }


        static bool EMPTY_CLAUSE(const Clause& c) { return (c.body->size() == 0  and (not c.head) ); }

        inline Clause* clone() const{
            auto* c = new Clause(head, new ClauseBody(body->begin(), body->end()));
            return c;
        }
        inline Clause* next() const{
            auto* c = new Clause(head, new ClauseBody(body->begin()+1, body->end()));
            c->setSubstitution(*theta + Substitution());
            return c;
        }
        /**
         * @brief Create a new renamed clause, used in a resolution step while resolving
         * a goal.
         * @param renamedBody.size() >= body->size()
         */ 
        void renameBody(ClauseBody& renamedBody, SuffixRenamer& vr) const {
            
            // renamed->body->resize()
            auto vset = VarSet();
            for (uint i =0;i < body->size() ; i++){
                auto& l = (*body)[i];
                const cnst_term_sptr& lr = (*l)(vr, vset);       //Apply renaming
                renamedBody[i] = *((cnst_lit_sptr*)&lr);
            }
            return;
        }

        std::size_t size()const { return body->size(); }

        void setHead(cnst_lit_sptr h){head = h;}
        
        const cnst_lit_sptr& getHead() const { 
            if ( head )return head; 
            return Term::null_literal_sptr;
        }

        Substitution& getSubstitution() const { return *theta;}

        void setSubstitution(Substitution* t){ theta = t;}

        cnst_lit_sptr& front() const { return (*body)[0];}

        void pop_front() { body->pop_front();}

        void delayFront(){
            body->front_to_back();
        }
      
        std::string to_string()const{
            std::string s("");
            if( body->size() > 0) s.append("(");
            if( head ) s.append( head->to_string() + " ");
            if( body->size() > 0) s.append(" <= ");

            for (auto &l : *body)
                s.append("\n" + l->to_string());

            if( body->size() > 0) s.append(")");
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