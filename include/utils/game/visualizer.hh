#ifndef VISUAZLIZER_HH
#define VISUAZLIZER_HH
#include "utils/game/match.hh"

namespace ares
{
    class visualizer
    {
        typedef std::unordered_map<int, std::string> cellMap;
    private:
        std::unordered_map<std::string , cellMap> board;
        const char * columns[8]{"a","b","c","d","e","f","g","h"};
    public:
        visualizer(/* args */) = default;
        void draw(const State& state);
        ~visualizer() {}
    };
} // namespace ares

#endif