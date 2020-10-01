#ifndef POLICIES_HH
#define POLICIES_HH
#include "montecarlo.hh"
#include "montecarlo_seq.hh"
namespace ares{

    class SelectionPolicy : public Montecarlo::ISelectionPolicy
    {
    private:
        Reasoner& reasoner;
    public:
        SelectionPolicy(Reasoner& r):reasoner(r) {}
        virtual Montecarlo:: Node* operator()(Montecarlo::Node*,const std::atomic_bool&)const;
        ~SelectionPolicy() {}
    };
    class SimPolicy : public Montecarlo::ISimPolicy
    {
    private:
        Reasoner& reasoner;
        
    public:
        SimPolicy(Reasoner& r):reasoner(r) {}
        /**
         * Start a a random simulation starting from node n and ending in a terminal node.
         * @param n a Node where the simulation will begin
         * @param indx the index of the role whose reward will be returned.
         */
        virtual float operator()(Montecarlo::Node* n,ushort indx,std::atomic_bool&)const;
        ~SimPolicy() {}
    };

     class SelectionPolicySeq : public MontecarloSeq::ISelectionPolicy
    {
    private:
        Reasoner& reasoner;
    public:
        SelectionPolicySeq(Reasoner& r):reasoner(r) {}
        virtual MontecarloSeq:: Node* operator()(MontecarloSeq::Node*,const std::atomic_bool&)const;
        ~SelectionPolicySeq() {}
    };
    class SimPolicySeq : public MontecarloSeq::ISimPolicy
    {
    private:
        Reasoner& reasoner;
        
    public:
        SimPolicySeq(Reasoner& r):reasoner(r) {}
        /**
         * Start a a random simulation starting from node n and ending in a terminal node.
         * @param n a Node where the simulation will begin
         * @param indx the index of the role whose reward will be returned.
         */
        virtual float operator()(MontecarloSeq::Node* n,ushort indx,std::atomic_bool&)const;
        ~SimPolicySeq() {}
    };
}
#endif