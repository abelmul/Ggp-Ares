#ifndef FACTORY_HH
#define FACTORY_HH
#include "utils/utils/hashing.hh"
namespace ares
{
   class Namer
   {
   private:
       /* data */
   public:
       Namer(/* args */) {}
       ~Namer() {}
       /**
         * Register names, and assign unique nums to them.
         */
        static inline ushort registerVname(const std::string& s){
            static ushort id=0;
            if( vNameId.find(s) != vNameId.end() ) return vNameId[s];
            vNameId[s] = id;
            vIdName[id] = s;
            return id++;
        }

        static inline ushort registerName(const std::string& s){
            static ushort id=60536;
            if( nameId.find(s) != nameId.end() ) return nameId[s];
            nameId[s] = id;
            idName[id] = s;
            return id++;
        }
        /**
         * Change the variable id @param id to its corresponding string anme
         */
        static inline std::string vname(const ushort& id){ return vIdName.at(id); }
        /**
         * Change the @param id of a literal,function,or constant 
         * to its corresponding name.
         * @returns the name of the term referenced by id
         */
        static inline std::string name(const ushort& id){ return idName.at(id); }
        /**
         * Change the name of a variable
         * @param string s its corresponding id.
         * @returns the id of the term with name s
         */
        static inline ushort vid(const std::string s){ return vNameId.at(s); }
        /**
         * Change the variable the id @param id to a versioned/renamed variable id.
         */
        static inline ushort idVers(const ushort& id, const ushort& ver){
            return (id + (ver * vIdName.size())); 
        }
        /**
         * Change the name of a literal,function,or constant 
         * @param string s its corresponding id.
         * @returns the id of the term with name s
         */
        static inline ushort id(const std::string s){ return nameId.at(s); }

    /**
     * Datas
     */ 
    private:
        static std::unordered_map<ushort, std::string> vIdName;
        static std::unordered_map<std::string, ushort> vNameId;

        static std::unordered_map<ushort, std::string> idName;
        static std::unordered_map<std::string, ushort> nameId;
    
    public:
        const static ushort ROLE;
        const static ushort INIT;
        const static ushort LEGAL;
        const static ushort NEXT;
        const static ushort TRUE;
        const static ushort DOES;
        const static ushort DISTINCT;
        const static ushort GOAL;
        const static ushort TERMINAL;
        const static ushort INPUT;
        const static ushort BASE;
        const static ushort X;
        const static ushort R;
   };
} // namespace ares

#endif