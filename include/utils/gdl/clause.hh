#ifndef CLAUSE_HH
#define CLAUSE_HH

#include "utils/gdl/literal.hh"
#include "reasoner/varRenamer.hh"

namespace Ares
{
    typedef std::vector<const Literal*> ClauseBody;

    class Clause
    {
    private:
        const Literal* head = nullptr;
        ClauseBody* _body = nullptr;
        ClauseBody& body;
        const Substitution* theta = nullptr;

    public:
        /**
         * Protect against accidental copying, pass by value,...
         */
        Clause(const Clause&) = delete;
        Clause(const Clause&&) = delete;
        Clause& operator =(const Clause&) = delete;
        Clause& operator =(const Clause&&) = delete;

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
        Clause* rename(VarRenamer& vr){
            return nullptr;
        }
        ClauseBody& getBody(){return body;}
        void setHead(const Literal* h){ if(!head) head = h;}
        void setBody(ClauseBody* b){ if(!_body) _body = b;}
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