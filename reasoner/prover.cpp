#include "reasoner/prover.hh"

namespace Ares
{
    bool Prover::prove(Clause& goal, bool one/*=true*/){
        return false;
    }
    Resolvent* Prover::resolve(Clause& goal, Clause& c){
        return nullptr;
    }
    Resolvent* Prover::handleNegation(Clause& goal){
        return nullptr;
    }
} // namespace Ares
