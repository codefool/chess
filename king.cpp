// Chess analysis
//
// Copyright (C) 2021 Garyl Hester. All rights reserved.
// github.com/codefool/chess
//
// Released under the GNU General Public Licence Version 3, 29 June 2007
//
#include "pieces.h"

std::vector<Dir> King::_d = {UP, DN, LFT,RGT,UPR,UPL,DNR,DNL};

MoveList King::getValidMoves(Board& b) {
    MoveList ret;

    std::vector<Pos> squares;
    b.getSquares(_p, _d, 1, squares);
    for (Pos p : squares)
        std::cout << p << std::endl;
    return ret;
}
