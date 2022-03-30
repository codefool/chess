//
// fenutil.cpp
//
// usage: fenutil -i <game_info> -p <population> -h <hi> -l <lo>
//                -f FEN string
//
#include <iostream>

#include "constants.h"

int main(int argc, char **argv)
{
    PositionPacked pp;
    std::string fen;

    for(int idx=1; idx < argc; ++idx)
    {
        std::string tok(argv[idx]);

        if (tok == "-i") {
            pp.gi.i = static_cast<uint32_t>(std::stoul(argv[++idx], NULL, 16));
        } else if(tok == "-p") {
            pp.pop = std::strtoull(argv[++idx], NULL, 16);
        } else if(tok == "-h") {
            pp.hi = std::strtoull(argv[++idx], NULL, 16);
        } else if(tok == "-l") {
            pp.lo = std::strtoull(argv[++idx], NULL, 16);
        } else if(tok == "-f") {
            bool first = true;
            tok = "";
            while(++idx < argc)
            {
                if (!first)
                    tok += ' ';
                tok += argv[idx];
                first = false;
            }
            std::cout << tok << std::endl;
            exit(0);
        } else {
            std::cout << "Unknown argument " << tok << std::endl;
            exit(1);
        }
    }
    std::cout << Position(pp).fen_string() << std::endl;
    return 0;
}