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
         * Rename all variables in context to $SuffixRenamer::suffix
         * Assuming no variable in gdl ends with $int
         * This class is not responsible for deleting the new 
         * variables created here as a result of renaming.
         */
        virtual Context* rename(Context& context){
            Context* renamed = new Context();
            //hold on to the lock just for enough time to increment suffix
            smutex.lock();
            ulong suf = SuffixRenamer::suffix++;
            smutex.unlock();
            for (auto &it : context.getMapping())
            {
                char* name = it.first->getName();
                int l = (int)(log10(suf)+1);
                int nLen = strlen(name);
                char* newName = (char*) malloc(nLen + l + 2);
                strcpy(newName, name);   
                sprintf(newName + nLen, "$%d",suf);
                renamed->bind(it.first,new Variable(newName));
            }
            return renamed;
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
