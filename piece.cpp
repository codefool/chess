// Chess analysis
//
// Copyright (C) 2021 Garyl Hester. All rights reserved.
// github.com/codefool/chess
//
// Released under the GNU General Public Licence Version 3, 29 June 2007
//
#include "dreid.h"

std::map<PieceType, const char *> Piece::s_n = {
	{ PT_EMPTY,    ".." },
	{ PT_KING,     "Kk" },
	{ PT_QUEEN,    "Qq" },
	{ PT_BISHOP,   "Bb" },
	{ PT_KNIGHT,   "Nn" },
	{ PT_ROOK,     "Rr" },
	{ PT_PAWN,     "Pp" },
	{ PT_PAWN_OFF, "Pp" }
};

std::map<unsigned char, short> Piece::s_z = {
	{ PT_KING              ,  0 },
	{ PT_QUEEN             ,  1 },
	{ PT_BISHOP            ,  2 },
	{ PT_KNIGHT            ,  3 },
	{ PT_ROOK              ,  4 },
	{ PT_PAWN              ,  5 },
	{ PT_KING   |BLACK_MASK,  6 },
	{ PT_QUEEN  |BLACK_MASK,  7 },
	{ PT_BISHOP |BLACK_MASK,  8 },
	{ PT_KNIGHT |BLACK_MASK, 19 },
	{ PT_ROOK   |BLACK_MASK, 10 },
	{ PT_PAWN   |BLACK_MASK, 11 },
};

Piece::Piece(PieceType t, Side s)
: _t{t}, _s{s}, _c{s_n[_t][_s]}
{}

bool Piece::is_on_move(Side m) const
{
    return _s == m;
}

PieceType Piece::getType() const
{
    return _t;
}

void Piece::setType(PieceType t)
{
    _t = t;
}

const char Piece::getPieceGlyph() const
{
    return _c;
}

Pos Piece::getPos() const
{
    return _p;
}

void Piece::setPos(Rank r, File f)
{
    _p.set(r,f);
}

void Piece::setPos(Pos p)
{
    _p = p;
}

bool Piece::isType(PieceType pt) const
{
    return _t == pt;
}

bool Piece::isEmpty() const
{
    return this == EMPTY.get();
}

const Side Piece::getSide() const
{
    return _s;
}

uint8_t Piece::toByte() const
{
	uint8_t r = static_cast<uint8_t>(_t);
	if (_s == SIDE_BLACK)
		r |= BLACK_MASK;
	return r;
}

short Piece::get_zob_idx()
{
    return Piece::s_z[toByte()];
}

PiecePtr Piece::create(PieceType pt, Side s)
{
	return std::make_shared<Piece>(pt, s);
}

PiecePtr Piece::EMPTY = Piece::create(PT_EMPTY, SIDE_WHITE);
