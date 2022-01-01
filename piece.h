// Chess analysis
//
// Copyright (C) 2021 Garyl Hester. All rights reserved.
// github.com/codefool/chess
//
// Released under the GNU General Public Licence Version 3, 29 June 2007
//
#pragma once

#include <map>
#include <memory>
#include <vector>

#include "constants.h"
#include "board.h"

// base class of all pieces

class Piece
{
private:
	PieceType _t;
	Side      _s;
	char	  _c;
	Pos		  _p;

private:
	static std::map<PieceType, const char *> s_n;

public:
	Piece(PieceType t, Side s)
	: _t{t}, _s{s}, _c{'\0'}
	{}

	bool is_on_move(Side m) { return _s == m; }

	PieceType getType() { return _t; }
	const char toChar() const {return _c;}

	const char getPieceGlyph(PieceType pt) {
		if (!_c)
			_c = s_n[pt][_s];
		return _c;
	}

	Pos& getPos() { return _p; }

	void setPos(Rank r, File f) {
		_p.set(r,f);
	}

	void setPos(Pos p) {
		_p = p;
	}

	Side getSide() { return _s; }

	// encode piece type and side into 4-bit value
	uint8_t toByte();

	static PiecePtr create(PieceType pt, Side s);
};
