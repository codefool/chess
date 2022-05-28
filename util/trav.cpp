// perform a board traversal

#include <iostream>
#include <vector>

#include "../dreid.h"

using namespace dreid;

int main(int argc, char **argv)
{
    // setup an initial position
    Position pos;
    pos.init();
    PositionPacked pp = pos.pack();

    std::vector<PositionPacked> seen;
    seen.push_back(pp);

    MoveList moves;
    moves.reserve(50);
    while( true )
    {
        Board sub_board(pp);
        Side  s = sub_board.gi().getOnMove();

        moves.clear();
        sub_board.get_all_moves(s, moves);
        if ( moves.size() == 0 )
            break;
        for ( MovePtr mv : moves )
        {
            Board brdPrime(sub_board.getPosition().pack());
            bool isPawnMove = brdPrime.process_move(mv, brdPrime.getPosition().gi().getOnMove());
            if ( brdPrime.gi().getPieceCnt() != 32)
                continue;
            brdPrime.getPosition().gi().toggleOnMove();
            pp = brdPrime.getPosition().pack();
            if ( std::find( std::begin(seen), std::end(seen), pp ) != std::end(seen) )
                continue;
            std::cout << brdPrime.getPosition().fen_string() << std::endl;
            seen.push_back( pp );

            break;
        }
    }
    std::cout << seen.size() << std::endl;

    return 0;
}