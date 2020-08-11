#include "reasoner/suffixRenamer.hh"
#include "utils/memory/memCache.hh"

namespace ares
{

    /**
     * Don't forget to initialize these!
     */
    MemCache* SuffixRenamer::pool = nullptr;
    std::atomic<ushort> SuffixRenamer::_suffix = 1;

   const cnst_term_sptr SuffixRenamer::get(const Variable* x) const {
        return  pool->getVar(Namer::idVers(x->get_name(), suffix));
    }
    
} // namespace Ares
