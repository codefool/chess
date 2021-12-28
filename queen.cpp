// Chess analysis
//
// Copyright (C) 2021 Garyl Hester. All rights reserved.
// github.com/codefool/chess
//
// Released under the GNU General Public Licence Version 3, 29 June 2007
//
#include "pieces.h"

std::vector<Dir> Queen::_d = {UP, DN, LFT,RGT,UPR,UPL,DNR,DNL};

MoveList Queen::getValidMoves() {
    return MoveList();
}
