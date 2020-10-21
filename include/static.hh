#ifndef STATIC_HH
#define STATIC_HH

#include "ares.hh"
#include "strategy/montecarlo.hh"
namespace ares
{
    // Create the static memory pool
    MemoryPool& mempool = MemoryPool::create(
        131072, 262144,
        {make_pair(1, 32768), make_pair(2, 32768), make_pair(3, 32768),
         make_pair(4, 32768), make_pair(5, 32768), make_pair(6, 32768),
         make_pair(7, 32768), make_pair(8, 32768), make_pair(9, 4096),
         make_pair(10, 32768), make_pair(11, 4096), make_pair(12, 4096)});
    tbb::concurrent_hash_map<ushort, std::vector<ThreadPool*>>
        ThreadPoolFactroy::pools;
    auto& tpf = ThreadPoolFactroy::create();
    /**
     * Static elements in Strategy.
     */
    std::unordered_set<std::string> Registrar::initd;
    std::unordered_map<std::string, Strategy*> Registrar::strategies;

    template <class T>
    Registrar RegistrarBase<T>::registrar(
        T::create());  // Rregistration of a new type

    UniqueVector<ushort> Montecarlo::order;

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

    // Namer static
    std::unordered_map<ushort, std::string> Namer::vIdName;
    std::unordered_map<std::string, ushort> Namer::vNameId;

    std::unordered_map<ushort, std::string> Namer::idName;
    std::unordered_map<std::string, ushort> Namer::nameId;

    ushort Namer::vid_ = 0;
    ushort Namer::id_ = 60536;
    /**
     * Reserve ids for known keywords.
     */
    ushort Namer::ROLE = Namer::registerName(std::string("role"));
    ushort Namer::OR = Namer::registerName(std::string("or"));
    ushort Namer::NOT = Namer::registerName(std::string("not"));
    ushort Namer::INIT = Namer::registerName(std::string("init"));
    ushort Namer::LEGAL = Namer::registerName(std::string("legal"));
    ushort Namer::NEXT = Namer::registerName(std::string("next"));
    ushort Namer::TRUE = Namer::registerName(std::string("true"));
    ushort Namer::DOES = Namer::registerName(std::string("does"));
    ushort Namer::DISTINCT = Namer::registerName(std::string("distinct"));
    ushort Namer::GOAL = Namer::registerName(std::string("goal"));
    ushort Namer::TERMINAL = Namer::registerName(std::string("terminal"));
    ushort Namer::INPUT = Namer::registerName(std::string("input"));
    ushort Namer::BASE = Namer::registerName(std::string("base"));
    ushort Namer::X = Namer::registerVname(std::string("?x"));
    ushort Namer::R = Namer::registerVname(std::string("?r"));

    template <class T>
    MemoryPool* _Body<T>::mempool = nullptr;

    std::ostream& operator<<(std::ostream& os, const Cfg& cfg)
    {
        os << cfg.str();
        return os;
    }
}  // namespace ares

#endif