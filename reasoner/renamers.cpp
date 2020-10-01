#include "reasoner/suffixRenamer.hh"
#include "utils/memory/memCache.hh"
#include "reasoner/variantRenamer.hh"
#include "ares.hh"
namespace ares
{

    /**
     * Don't forget to initialize these!
     */

   const Term* SuffixRenamer::get(const Variable* x) const {
        return  Ares::memCache->getVar(Namer::idVers(x->get_name(), suffix));
    }

    //Methods of VaraintRenamer
    const Term* VariantRenamer::get(const Variable* x) const {
        ushort& renamed = renaming[x->get_name()];
        if( !renamed )
            renamed = nxt++;
        
        return Ares::memCache->getVar(renamed);
    }
} // namespace Ares
