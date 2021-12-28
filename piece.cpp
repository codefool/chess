// Chess analysis
//
// Copyright (C) 2021 Garyl Hester. All rights reserved.
// github.com/codefool/chess
//
// Released under the GNU General Public Licence Version 3, 29 June 2007
//
#include "pieces.h"

std::map<Dir,Offset> Piece::s_os = {
	{UP, {+0,+1}},
	{DN, {+0,-1}},
	{LFT,{-1,+0}},
	{RGT,{+1,+0}},
	{UPR,{+1,+1}},
	{UPL,{-1,+1}},
	{DNR,{+1,-1}},
	{DNL,{-1,-1}}
};

std::shared_ptr<Piece> Piece::create(PieceType pt, bool s)
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
