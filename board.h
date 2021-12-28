// Chess analysis
//
// Copyright (C) 2021 Garyl Hester. All rights reserved.
// github.com/codefool/chess
//
// Released under the GNU General Public Licence Version 3, 29 June 2007
//
#pragma once

#include <cstdint>
#include <map>
#include <memory>

#include "constants.h"
#include "gameinfo.h"

class Board {
private:
	uint8_t  _b[8][8];
	// we always need to know where the kings are
	PiecePtr _k[2];
	PiecePtr _wk;
	GameInfo _gi;
	std::map<uint8_t,PiecePtr> _p;

public:
	Board();
	GameInfo& gi() { return _gi; }

	void placePiece(PieceType t, Side s, Rank r, File f);
	bool inBounds(short f, short r);

	// a property of the physical chessboard is that
	// if the odd'ness of the rank and file are the
	// same, then the square is black, otherwise it's
	// white.
	bool isBlackSquare(short r, short f);

	void dump();
};
