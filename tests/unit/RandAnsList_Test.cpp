#include "common.hh"
#include "reasoner/cache.hh"
#include <unordered_set>

using namespace ares;
void setup();
void RandItTest();

namespace ares{
    std::atomic<int> Query::nextId = 0;
    AnsIterator Cache::NOT_CACHED(nullptr,-1,nullptr);

    std::random_device RandomAnsIterator::rd;
    std::mt19937 RandomAnsIterator::gen(RandomAnsIterator::rd());
}

int main(int argc, char const *argv[])
{
    setup();
    Runner runner;
    add_test(runner, RandItTest);
    runner();
    return 0;
}

void RandItTest(){

}