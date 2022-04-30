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

#include "dreid.h"

// base class of all pieces

class Piece
{
private:
	PieceType _t;
	Side      _s;
	char	  _c;
	short	  _p;

private:
	static std::map<PieceType, const char *> s_n;

public:
	Piece(PieceType t, Side s)
	: _t{t}, _s{s}, _c{s_n[_t][_s]}
	{}

	bool is_on_move(Side m) { return _s == m; }

	PieceType getType() { return _t; }
	void setType(PieceType t) { _t = t; }

	const char getPieceGlyph() const { return _c; }

	short getPos() { return _p; }

	void setPos(Rank r, File f) {
		_p = ((r << 3)|f);
	}

	void setPos(short p) {
		_p = p;
	}

	bool isType(PieceType pt) const {
		return _t == pt;
	}

	bool isEmpty() const {
		return this == EMPTY.get();
	}

	Side getSide() { return _s; }

	// pack piece type and side into 4-bit value
	uint8_t toByte();

	static PiecePtr create(PieceType pt, Side s);
	static PiecePtr EMPTY;
};
