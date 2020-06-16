#ifndef CLAUSE_HH
#define CLAUSE_HH

#include "utils/gdl/literal.hh"
#include "reasoner/substitution.hh"
#include "reasoner/suffixRenamer.hh"

namespace Ares
{
    /**
     * Apparently std::vector is faster than std::list even for situations 
     * involving lots of insertions and deletions!
     */
    typedef std::vector<const Literal*> ClauseBody; 

    class Clause
    {
        friend class Transformer;
    private:
        const Literal* head = nullptr;
        ClauseBody* _body = nullptr;
        ClauseBody& body;
        Substitution* theta = nullptr;
        ClauseBody& getBody(){return body;}

    public:
        /**
         * Protect against accidental copying, pass by value,...
         */
        Clause(const Clause&) = delete;
        Clause(const Clause&&) = delete;
        Clause& operator =(const Clause&) = delete;
        Clause& operator =(const Clause&&) = delete;

        static bool EMPTY_CLAUSE(const Clause& c) { return (c.body.size() == 0  and (not c.head) ); }
        /**
         * A clause has this kind of form:
         * A <- A0 and ... and An
         * where A0...An are literals(the body), and A is the head
         */
        Clause(const Literal* head,ClauseBody* _b)
        :head(head),_body(_b),body(std::ref(*_body))
        {
        };

        Clause* clone(){
            return new Clause(head, new ClauseBody(body.begin(), body.end()));
        }
        /**
         * Create a new renamed clause, used in a resolution step while resolving
         * a goal.
         */ 
        Clause* rename() const {
            SuffixRenamer vr;
            Clause* renamed = new Clause(head, new ClauseBody());
            // renamed->body.resize()
            auto vset = VarSet();
            for (auto &&l : body){
                auto lr = (*l)( *((Substitution*)&vr), vset);       //Apply renaming
                renamed->body.push_back(lr);
            }
            return renamed;
        }

        std::size_t size()const { return body.size(); }

        void setHead(const Literal* h){head = h;}
        const Literal& getHead() const { return *head;}
        // void setBody(ClauseBody* b){ if(!_body) _body = b;}

        Substitution* getSubstitution() const { return theta;}
        void setSubstitution(Substitution* t){ theta = t;}

        const Literal& front() const { return *body[0];}

        void insertFront(const Literal* l) { body.insert(body.begin(),l); }
        void popFront() { body.erase(body.begin());}

        void delayFront(){
            const Literal* l = body.front();
            popFront();
            body.push_back(l);
        }

        Clause& operator+=(const Clause& c){
            //Experiment with both inserting at the back and the front
            this->body.insert(body.begin(), c.body.begin()+1, c.body.end());
            return *this;
        }

        std::string toString()const{
            std::string s("(");
            if( body.size() > 0) s.append(" <= ");
            if( head ) s.append( head ->toString() + " ");
            for (auto &l : body){
                s.append("\n" + l->toString());
            }
            s.append(")");
            return s;
        }
        friend std::ostream & operator << (std::ostream &out, const Clause &c){
            out << c.toString();
            return out;
        }
        ~Clause(){
            //A literal is not unique to just a clause, its shared b/n clauses, managed by ExpressionPool.
            if (theta) delete theta;
            if( body.size() != 0) delete _body; //Don't delete gdlParser::EMPTY_BODY;
        }
    };
    
} // namespace Ares

#endif