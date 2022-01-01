// Chess analysis
//
// Copyright (C) 2021 Garyl Hester. All rights reserved.
// github.com/codefool/chess
//
// Released under the GNU General Public Licence Version 3, 29 June 2007
//
#include "piece.h"

std::map<PieceType, const char *> s_n = {
	{ PT_KING,     "Kk" },
	{ PT_QUEEN,    "Qq" },
	{ PT_BISHOP,   "Bb" },
	{ PT_KNIGHT,   "Nn" },
	{ PT_ROOK,     "Rr" },
	{ PT_PAWN,     "Pp" },
	{ PT_PAWN_OFF, "Pp" }
};

uint8_t Piece::toByte() {
	uint8_t r = static_cast<uint8_t>(_t);
	if (_s == SIDE_BLACK)
		r |= BLACK_MASK;
	return r;
}

PiecePtr Piece::create(PieceType pt, Side s)
{
	return std::make_shared<Piece>(pt, s);
}
