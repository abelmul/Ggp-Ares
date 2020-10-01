#ifndef STATIC_HH
#define STATIC_HH

#include "ares.hh"
namespace ares
{
    //Create the static memory pool
    MemoryPool& mempool = MemoryPool::create(131072,262144,{make_pair(1,32768),make_pair(2,32768),make_pair(3,32768),make_pair(4,32768),make_pair(5,32768),make_pair(6,32768),make_pair(7,32768),make_pair(8,32768),make_pair(9,4096),make_pair(10,32768),make_pair(11,4096),make_pair(12,4096)});
    tbb::concurrent_hash_map<ushort,std::vector<ThreadPool*>> ThreadPoolFactroy::pools;
    auto& tpf  = ThreadPoolFactroy::create();
    /**
     * Static elements in Strategy.
     */
    std::unordered_set<std::string> Registrar::initd;
    std::unordered_map<std::string, Strategy*> Registrar::strategies;

    template <class T>
    Registrar RegistrarBase<T>::registrar(T::create());    //Rregistration of a new type
    

    Reasoner* Strategy::reasoner;
    /**
     * AnswerIterator
     */
    // std::random_device RandomAnsIterator::rd;
    // std::mt19937 RandomAnsIterator::gen(RandomAnsIterator::rd());

    // template <class T>
    // std::random_device RandIterator<T>::rd;
    
    
   

    Prover* ClauseCB::prover;
    MemCache* Ares::memCache = nullptr;
    MemoryPool* Ares::mempool = nullptr;
    
    //Namer static
    std::unordered_map<ushort, std::string> Namer::vIdName;
    std::unordered_map<std::string, ushort> Namer::vNameId;
    
    std::unordered_map<ushort, std::string> Namer::idName;
    std::unordered_map<std::string, ushort> Namer::nameId;

    /**
     * Reserve ids for known keywords.
     */
    const ushort Namer::ROLE = Namer::registerName(std::string("role"));
    const ushort Namer::OR = Namer::registerName(std::string("or"));
    const ushort Namer::INIT = Namer::registerName(std::string("init"));
    const ushort Namer::LEGAL = Namer::registerName(std::string("legal"));
    const ushort Namer::NEXT = Namer::registerName(std::string("next"));
    const ushort Namer::TRUE = Namer::registerName(std::string("true"));
    const ushort Namer::DOES = Namer::registerName(std::string("does"));
    const ushort Namer::DISTINCT = Namer::registerName(std::string("distinct"));
    const ushort Namer::GOAL = Namer::registerName(std::string("goal"));
    const ushort Namer::TERMINAL = Namer::registerName(std::string("terminal"));
    const ushort Namer::INPUT = Namer::registerName(std::string("input"));
    const ushort Namer::BASE = Namer::registerName(std::string("base"));
    const ushort Namer::X = Namer::registerVname(std::string("?x"));
    const ushort Namer::R = Namer::registerVname(std::string("?r"));
    
    template<class T>
    MemoryPool* _Body<T>::mempool =nullptr;

    std::ostream& operator<<(std::ostream& os, const Cfg& cfg){
        os << cfg.str();
        return os;
    }
} // namespace ares

#endif