#include "reasoner/suffixRenamer.hh"
#include "utils/memory/expressionPool.hh"

namespace ares
{

    /**
     * Don't forget to initialize these!
     */
    ExpressionPool* SuffixRenamer::pool = nullptr;
    
   const cnst_term_sptr SuffixRenamer::get(cnst_var_sptr& x) const {
        uint ol = strlen(x->get_name());
        uint l = ol + (( (int) log10f(suffix) ) + 1 ) + 2;
        std::shared_ptr<char> newName((char * ) malloc( l ));
        strcpy(newName.get(), x->get_name());
        sprintf(newName.get() + ol, "$%d", suffix);
        return  pool->getVar(newName.get());
    }
    
} // namespace Ares
