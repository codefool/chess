// Chess analysis
//
// Copyright (C) 2021 Garyl Hester. All rights reserved.
// github.com/codefool/chess
//
// Released under the GNU General Public Licence Version 3, 29 June 2007
//
#include "pieces.h"

std::vector<Offset> Knight::_o = {
	{+1,-2}, {+1,+2},
	{+2,-1}, {+2,+1},
	{-2,-1}, {-2,+1},
	{-1,-2}, {-1,+2}
};

MoveList Knight::getValidMoves(Board& b) {
    return MoveList();
}
