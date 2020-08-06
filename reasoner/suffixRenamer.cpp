#include "reasoner/suffixRenamer.hh"
#include "utils/memory/expressionPool.hh"

namespace ares
{

    /**
     * Don't forget to initialize these!
     */
    ExpressionPool* SuffixRenamer::pool = nullptr;
    
   const cnst_term_sptr SuffixRenamer::get(const cnst_var_sptr& x) const {
        return  pool->getVar(Namer::idVers(x->get_name(), suffix));
    }
    
} // namespace Ares
