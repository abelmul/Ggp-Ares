#ifndef FACTORY_HH
#define FACTORY_HH
#include "utils/gdl/gdl.hh"
namespace ares
{
   class Factory
   {
   private:
       /* data */
   public:
       Factory(/* args */) {}
       ~Factory() {}
        /**
         * Simple factory methods for terms.
         */
        inline Function* getFn(){
            // return new Function
        }
        inline Literal* getLit(){

        }
        inline Variable* getVar(){
            
        }
        inline Constant* getConst(){
            
        }
   };
} // namespace ares

#endif