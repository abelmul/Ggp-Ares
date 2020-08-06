#include "utils/game/visualizer.hh"
#include <boost/format.hpp>
#include "utils/memory/namer.hh"
namespace ares
{
    void visualizer::draw(const State& state){
        auto* s = state[Namer::TRUE];
        board.clear();
        for (auto &&trues : *s)
        {
            auto* propos = trues->head->body[0].get();
            if( propos->get_name() != Namer::id("cell") )  continue;
            const Function* cell =  ((const Function*)propos);
            auto col = Namer::name(cell->body[0]->get_name());
            int row = std::atoi(Namer::name(cell->body[1]->get_name()).c_str());
            std::string player ( Namer::name(cell->body[2]->get_name()).c_str());
            player.append(" ");
            if( cell->body.size() == 4 )player.append(Namer::name(cell->body[3]->get_name()).c_str());
            board[col][row] = player;
        }
        std::cout << "\n  |";
        for (size_t i = 0; i < 8; i++)
                std::cout << boost::format("%|=15|") % "---------------" <<"|" ;
                
        for (int row = 8; row > 0; row--)
        {
            std::cout << "\n" <<row << " |";
            for (size_t col = 0; col < 8; col++)
                std::cout << boost::format("%|=15|") % board[columns[col]][row] << "|";
            
            std::cout << "\n  |";
            for (size_t i = 0; i < 8; i++)
                std::cout << boost::format("%|=15|") % "_______________" <<"|" ;
            if( row == 1) continue;
            std::cout << "\n  |";
            for (size_t i = 0; i < 8; i++)
                std::cout << boost::format("%|=15|") % " " <<"|" ;
        }
        std::cout << "\n   ";
        for (size_t i = 0; i < 8; i++)
            std::cout << boost::format("%|=15|") % columns[i] <<" ";
        
        std::cout << "\n";
        
    }
} // namespace ares