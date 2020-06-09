#include "reasoner/varRenamer.hh"
#include <string.h>

namespace Ares
{
    /**
     * Renames variables by adding a unique suffix to them.
     */
    class SuffixRenamer : VarRenamer
    {
    private:
        SuffixRenamer(){}
        static SuffixRenamer* _renamer;
        static std::mutex smutex;
        static uint suffix;

    public:
        /**
         * Rename all variables in Substitution to $SuffixRenamer::suffix
         * Assuming no variable in gdl ends with $int
         * This class is not responsible for deleting the new 
         * variables created here as a result of renaming.
         */
        virtual Substitution* rename(Substitution& sub){
            return nullptr;
        }
        static SuffixRenamer* getRenamer(){
            //Lock the mutex, lock guard automatically releases lock when destroyed
            std::lock_guard<std::mutex> g(smutex);
            if(!_renamer)
                _renamer = new SuffixRenamer();
            
            return _renamer;
        }
    };
    SuffixRenamer* SuffixRenamer::_renamer=nullptr;
    std::mutex SuffixRenamer::smutex;
    uint SuffixRenamer::suffix = 0; 

} // namespace Ares
