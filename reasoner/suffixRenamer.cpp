#include "reasoner/suffixRenamer.hh"
#include "utils/gdl/gdlParser/expressionPool.hh"

namespace Ares
{

    /**
     * Don't forget to initialize these!
     */
    ExpressionPool* SuffixRenamer::pool = nullptr;
    std::mutex SuffixRenamer::smutex;
    uint SuffixRenamer::suffix = 0;
    
    const Term* SuffixRenamer::get(const Variable* x) const {
        uint ol = strlen(x->getName());
        uint l = ol + (( (int) log10f(current_suffix) ) + 1 ) + 1;
        char* newName = (char * ) malloc( l );
        strcpy(newName, x->getName());
        sprintf(newName + ol, "%d", current_suffix);
        const Variable* x_r = pool->getVar(newName);
        delete newName;
        return x_r;
    }
} // namespace Ares
