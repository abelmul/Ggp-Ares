#include "utils/utils/iterators.hh"
#include "reasoner/reasoner.hh"

namespace ares{
    void ActionIterator::init(){
        actions = reasoner->actions(*state);
    }
}