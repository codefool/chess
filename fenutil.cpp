//
// fenutil.cpp
//
// usage: fenutil fen <game_info> <population> <hi> <lo>
//                pos FEN string
//
#include <iostream>

#include "constants.h"

void usage()
{
    std::cerr << "FEN utility\n"
                 "usage: fenutil command [data]\n"
                 "\twhere command is:\n"
                 "\tfen game_info population hi lo\n"
                 "\tpos FEN string"
              << std::endl;
    exit(1);
}

int main(int argc, char **argv)
{
    if (argc < 3)
        usage();

    std::string cmd(argv[1]);
    if (cmd == "fen") {
        if (argc < 6)
            usage();
        PositionPacked pp;
        pp.gi.i = static_cast<uint32_t>(std::stoul(argv[2], NULL, 16));
        pp.pop  = std::strtoull(argv[3], NULL, 16);
        pp.hi   = std::strtoull(argv[4], NULL, 16);
        pp.lo   = std::strtoull(argv[5], NULL, 16);
        std::cout << Position(pp).fen_string() << std::endl;
    } else if (cmd == "pos") {
        std::string fen;
        bool first = true;
        for(int idx{2}; idx < argc; idx++)
        {
            if (!first)
                fen += ' ';
            fen += argv[idx];
            first = false;
        }
        std::cout << fen << std::endl;
    } else {
        usage();
    }
    return 0;
}