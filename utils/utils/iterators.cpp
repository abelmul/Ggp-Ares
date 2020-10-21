#include "reasoner/reasoner.hh"
#include "utils/utils/iterators.hh"

namespace ares
{
    void ActionIterator::init() { actions = reasoner->actions(*state); }
}  // namespace ares