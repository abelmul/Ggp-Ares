#ifndef NAMER_HH
#define NAMER_HH
#include "utils/utils/hashing.hh"
#include <boost/algorithm/string/case_conv.hpp>
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
            if( vNameId.find(s) != vNameId.end() ) return vNameId[s];
            const auto& sLower = boost::to_lower_copy(s);
            vNameId[sLower] = vid_;
            vIdName[vid_] = sLower;
            return vid_++;
        }

        static inline ushort registerName(const std::string& s){
            if( nameId.find(s) != nameId.end() ) return nameId[s];
            const auto& sLower = boost::to_lower_copy(s);
            nameId[sLower] = id_;
            idName[id_] = sLower;
            return id_++;
        }
        static inline void reset(){
            id_=60536;
            vid_=0;
            vIdName.clear();
            vNameId.clear();
            idName.clear();
            nameId.clear(); 
            ROLE = registerName(std::string("role"));
            OR = registerName(std::string("or"));
            NOT = registerName(std::string("not"));
            INIT = registerName(std::string("init"));
            LEGAL = registerName(std::string("legal"));
            NEXT = registerName(std::string("next"));
            TRUE = registerName(std::string("true"));
            DOES = registerName(std::string("does"));
            DISTINCT = registerName(std::string("distinct"));
            GOAL = registerName(std::string("goal"));
            TERMINAL = registerName(std::string("terminal"));
            INPUT = registerName(std::string("input"));
            BASE = registerName(std::string("base"));
            X = registerVname(std::string("?x"));
            R = registerVname(std::string("?r"));
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
        static ushort vid_;
        static ushort id_;
    
    public:
        static ushort ROLE;
        static ushort OR;
        static ushort NOT;
        static ushort INIT;
        static ushort LEGAL;
        static ushort NEXT;
        static ushort TRUE;
        static ushort DOES;
        static ushort DISTINCT;
        static ushort GOAL;
        static ushort TERMINAL;
        static ushort INPUT;
        static ushort BASE;
        static ushort X;
        static ushort R;
   };
} // namespace ares

#endif