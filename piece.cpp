// Chess analysis
//
// Copyright (C) 2021 Garyl Hester. All rights reserved.
// github.com/codefool/chess
//
// Released under the GNU General Public Licence Version 3, 29 June 2007
//
#include "pieces.h"

uint8_t Piece::toByte() {
	uint8_t r = static_cast<uint8_t>(_t);
	if (_s == SIDE_BLACK)
		r |= BLACK_MASK;
	return r;
}

PiecePtr Piece::create(PieceType pt, Side s)
{
	switch(pt) {
		case PT_KING:	  return std::make_shared<King>(s);
		case PT_QUEEN:	  return std::make_shared<Queen>(s);
		case PT_BISHOP:   return std::make_shared<Bishop>(s);
		case PT_KNIGHT:   return std::make_shared<Knight>(s);
		case PT_ROOK:     return std::make_shared<Rook>(s);
		case PT_PAWN:
		case PT_PAWN_OFF: return std::make_shared<Pawn>(s, pt==PT_PAWN_OFF);
	}
	return nullptr;
}
