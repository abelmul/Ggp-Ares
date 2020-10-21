#include "ares.hh"
#include "reasoner/suffixRenamer.hh"
#include "utils/memory/memCache.hh"
namespace ares
{
    /**
     * Don't forget to initialize these!
     */

    const Term* SuffixRenamer::get(const Variable* x) const
    {
        return Ares::memCache->getVar(Namer::idVers(x->get_name(), suffix));
    }
}  // namespace ares
