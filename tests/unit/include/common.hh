#ifndef COMMON_HH
#define COMMON_HH
#include "mocks.hh"
#include "runner.hh"
namespace ares{
    Ares aresP;
    Cfg cfg("./ares.cfg.json");
};

void setup(){
    using namespace ares;
    srand(time(NULL));
    aresP.mempool = &MemoryPool::create(100,100,std::vector<std::pair<arity_t,uint>>());
    aresP.memCache = aresP.mempool->getCache();
    Body::mempool = ClauseBody::mempool = aresP.mempool;
}

/**
 * 
 * Some utilities, useful for most testings.
 * 
 */
namespace ares{

    std::unordered_map<ushort, ushort> fnArityMap;
    std::unordered_map<ushort, ushort> litArityMap;

    std::random_device rd; // obtain a random number from hardware
    std::mt19937 gen(rd()); // seed the generator
    std::binomial_distribution<int> bd(2,0.5);

    typedef UniqueVector<const Variable*> OrdrdVarSet;
    typedef std::pair<const Atom*,OrdrdVarSet> lit_var_pair;
    typedef const Term*(*RandFactory)(OrdrdVarSet&,ushort&,ushort);

    void extractVar(const structured_term* l,OrdrdVarSet&);
    const Term* getRandConst(OrdrdVarSet&,ushort&,ushort max_arity=0);
    const Term* getRandVar(OrdrdVarSet&,ushort&,ushort max_arity=0);
    const Term* getRandFn(OrdrdVarSet&,ushort&,ushort max_arity=0);

    ushort getRandName(uint min, uint max){
        std::random_device rd; // obtain a random number from hardware
        std::mt19937 gen(rd()); // seed the generator
        std::uniform_int_distribution<> distr(min, max); // define the range
        return distr(gen);
    }
    Body* getRandBody(OrdrdVarSet& vars,ushort arity,ushort& depth,ushort max_arity=3){
        RandFactory  randF[] = {getRandConst,getRandVar,getRandFn};
        Body* body = new Body(arity);
        for (size_t i = 0; i < arity; i++)
        {
            ushort cur_depth = depth;
            ushort type = bd(gen);
            if( depth == 0){
                type = rand() %2;
            }
            (*body)[i] = randF[type](vars,cur_depth,max_arity);
        }
        return body;
    }
    const Term* getRandConst(OrdrdVarSet&,ushort&,ushort){
        return Ares::memCache->getConst(getRandName(20,55));
    }
    const Term* getRandVar(OrdrdVarSet& vars,ushort&,ushort){
        auto v =  Ares::memCache->getVar(getRandName(0,5));
        vars.push_back(v);
        return v;
    }
    const Term* getRandFn(OrdrdVarSet& vars,ushort& depth,ushort max_arity){
        PoolKey key;
        depth--;
        key.name = getRandName(60, 80);
        ushort arity = (rand() % max_arity) + 1;
        if( fnArityMap.find(key.name) == fnArityMap.end() )
            fnArityMap[key.name] = arity;
        else
            arity = fnArityMap[key.name];
        key.body = getRandBody(vars,arity, depth, max_arity);
        return Ares::memCache->getFn(key);
    }
    lit_var_pair getRandLiteral(ushort depth,ushort max_arity=3,ushort max_arity_sb=3){
        PoolKey key;
        key.name = getRandName(81, 100);
        ushort arity = (rand() % max_arity);
        if( litArityMap.find(key.name) == litArityMap.end() )
            litArityMap[key.name] = arity;
        else
            arity = litArityMap[key.name];
        
        OrdrdVarSet vars;
        key.body = getRandBody(vars,arity,depth, max_arity_sb);
        return lit_var_pair(Ares::memCache->getAtom(key), vars);
    }
    Substitution* getRandVariant(const Atom* l){
        ushort depth = (rand() % 2) +1;
        VariantSubstitution* sigma = new VariantSubstitution();
        OrdrdVarSet vars;
        extractVar(l,vars);
        OrdrdVarSet varsnew;
        std::unordered_set<const Variable*> seen;
        for (auto &&v : vars){
            const Term* vp = getRandVar(varsnew,depth);
            while (seen.find((Variable*)vp) != seen.end()){
                vp = getRandVar(varsnew,depth);
            }
            sigma->bind(v, vp);
            seen.insert((Variable*)vp);
        }
        return sigma;
    }
    std::unique_ptr<Clause> getRandClause(ushort max_size=15){
        ushort size =( rand() % max_size);
        ClauseBody* body = new ClauseBody(size);
        for (size_t i = 0; i < size; i++)
        {
            ushort depth = (rand() % 2) +1;
            ushort max_arity = (rand() % 5) + 2;
            (*body)[i] = getRandLiteral(depth, max_arity).first;
        }
        return std::unique_ptr<Clause>(new Clause(nullptr, body));
    }
    void extractVar(const structured_term* l,OrdrdVarSet& v){
        for (auto &&i : l->getBody())
        {
            if( is_var(i) )  v.push_back((Variable*)i);
            else if( is_struct_term(i) ) extractVar((structured_term*)i,v);
        }
    }
};


// #define assert_true.count _assert_true.count
#endif