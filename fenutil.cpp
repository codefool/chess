//
// fenutil.cpp
//
// usage: fenutil fen <game_info> <population> <hi> <lo>
//                pos FEN string
//
#include <iostream>

#include "dreid.h"

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

    std::vector<std::string> sargs(6);
    for(int i(1); i < argc; ++i )
        sargs[i-1] = argv[i];

    if (sargs[0] == "fen") {
        if (argc == 3 ) {
            std::string tmp(sargs[1]);
            int idx(0);
            size_t beg(0);
            size_t pos = tmp.find(',');
            if ( pos == std::string::npos )
                usage();
            sargs[1] = tmp.substr(beg,pos);
            beg = pos + 1;
            pos = tmp.find(',',beg);
            if ( pos == std::string::npos )
                usage();
            sargs[2] = tmp.substr(beg,pos-beg);
            beg = pos + 1;
            pos = tmp.find(',',beg);
            if ( pos == std::string::npos )
                usage();
            sargs[3] = tmp.substr(beg,pos-beg);
            beg = pos + 1;
            pos = tmp.find(',',beg);
            if ( pos != std::string::npos )
                usage();
            sargs[4] = tmp.substr(beg);
        } else if (argc < 6)
            usage();
        PositionPacked pp;
        pp.gi.i = static_cast<uint32_t>(std::stoul(sargs[1], NULL, 16));
        pp.pop  = std::strtoull(sargs[2].c_str(), NULL, 16);
        pp.hi   = std::strtoull(sargs[3].c_str(), NULL, 16);
        pp.lo   = std::strtoull(sargs[4].c_str(), NULL, 16);
        std::cout << Position(pp).fen_string() << std::endl;
    } else if (sargs[0] == "pos") {
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