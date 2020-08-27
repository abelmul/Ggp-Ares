#include "ares.hh"
#include "utils/utils/httpHandler.hh"

ares::Cfg ares::cfg;

int main(int argc, char const *argv[])
{
    std::cout.setf(std::ios::unitbuf);
    using namespace ares;
    //Read in the configuration file
    cfg = Cfg("./ares.cfg.json");

    //Create the memory pool
    std::vector<std::pair<arity_t, uint>> arities = {make_pair(1,32768),make_pair(2,32768),make_pair(3,32768),make_pair(4,32768),make_pair(5,32768),make_pair(6,32768),make_pair(7,32768),make_pair(8,32768),make_pair(9,4096),make_pair(10,32768),make_pair(11,4096),make_pair(12,4096)};
    MemoryPool* mempool = new MemoryPool(131072,262144,arities);
    Ares::setMem(mempool);

    //Get the singleton prover
    Prover* prover(Prover::getProver());
    ClauseCB::prover = prover;

    //Setup some static elements
    Body::mempool = ClauseBody::mempool = mempool;
    SuffixRenamer::setPool(mempool->getCache());

    //Create the parser
    GdlParser* p = GdlParser::getParser(cfg.parserThreads,mempool->getCache());

    //Create a reasoner over a game
    Reasoner* reasoner(new Reasoner(*p, prover,*mempool->getCache()));

    //Create Ares
    std::unique_ptr<Ares> ares( Ares::getAres(reasoner, p));

    //Start the server!
    HttpHandler handler(*ares,std::string("http://localhost:8080"));
    
    //Just wait
    std::mutex mWait;
    std::condition_variable cvWait;
    std::unique_lock<std::mutex> lk(mWait);
    cvWait.wait(lk);
    return 0;
}   
// voi
namespace ares
{
    SpinLock Ares::sl;
    Ares* Ares::ares = nullptr;
    Prover* ClauseCB::prover;
    MemCache* Ares::memCache = nullptr;
    MemoryPool* Ares::mempool = nullptr;
    
    //Initialize static members of term
    cnst_term_sptr     Term::null_term_sptr(nullptr);
    cnst_lit_sptr     Term::null_literal_sptr(nullptr);
    
    //Namer static
    std::unordered_map<ushort, std::string> Namer::vIdName;
    std::unordered_map<std::string, ushort> Namer::vNameId;
    
    std::unordered_map<ushort, std::string> Namer::idName;
    std::unordered_map<std::string, ushort> Namer::nameId;

    /**
     * Reserve ids for known keywords.
     */
    const ushort Namer::ROLE = Namer::registerName(std::string("role"));
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
