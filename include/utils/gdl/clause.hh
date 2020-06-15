#ifndef CLAUSE_HH
#define CLAUSE_HH

#include "utils/gdl/literal.hh"
#include "reasoner/varRenamer.hh"
#include <list>
namespace Ares
{
    typedef std::list<const Literal*> ClauseBody;

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

        static bool EMPTY_CLAUSE(const Clause& c) { return c.body.size() == 0; }
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
        Clause* rename(VarRenamer& vr){ return nullptr; }

        std::size_t size(){ return body.size(); }

        void setHead(const Literal* h){ if(!head) head = h;}
        // void setBody(ClauseBody* b){ if(!_body) _body = b;}

        Substitution* getSubstitution(){ return theta;}

        const Literal& front() { return *body.front();}

        void insertFront(const Literal* l) { body.push_front(l); }
        void popFront() { body.pop_front();}

        void delayFront(){
            const Literal* l = body.front();
            body.pop_front();
            body.push_back(l);
        }
        std::string toString()const{
            std::string s("(");
            if( body.size() > 0) s.append(" <= ");
            for (auto &l : body){
                s.append(" " + l->toString());
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