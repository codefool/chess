// Chess analysis
//
// Copyright (C) 2021 Garyl Hester. All rights reserved.
// github.com/codefool/chess
//
// Released under the GNU General Public Licence Version 3, 29 June 2007
//
#pragma once

#include <vector>

enum DrawReason {
	R_NONE                = 0x00,
	//
	R_14A_STALEMATE       = 0x00,
	R_14C_TRIPLE_OF_POS   = 0x01,
	R_14D_NO_MATERIAL     = 0x02,
	R_14F_50_MOVE_RULE    = 0x03
};

enum Draw14dReason {
	R_14D_NONE            = 0x00,
	//
	R_14D1_KVK            = 0x00,
	R_14D2_KVKWBN         = 0x01,
	R_14D3_KWBVKWB        = 0x02,
	R_14D4_NO_LGL_MV_CM   = 0x03
};

enum Rank {
	R1 = 0x00,
	R2 = 0x01,
	R3 = 0x02,
	R4 = 0x03,
	R5 = 0x04,
	R6 = 0x05,
	R7 = 0x06,
	R8 = 0x07
};

enum File {
	Fa = 0x00,
	Fb = 0x01,
	Fc = 0x02,
	Fd = 0x03,
	Fe = 0x04,
	Ff = 0x05,
	Fg = 0x06,
	Fh = 0x07
};

enum PieceType {
	PT_EMPTY     = 0x00,
	PT_KING      = 0x01,
	PT_QUEEN     = 0x02,
	PT_BISHOP    = 0x03,
	PT_KNIGHT    = 0x04,
	PT_ROOK      = 0x05,
	PT_PAWN      = 0x06, // on its own file
	PT_PAWN_OFF  = 0x07,  // off its own file
	SIDE_MASK    = 0x08,
	SIDE_WHITE   = 0x00,
	SIDE_BLACK   = 0x08,
	PIECE_MASK   = 0x07
};

enum Dir {
	UP = 0,
	DN,
	LFT,
	RGT,
	UPR,
	UPL,
	DNR,
	DNL
};

struct Offset {
	short df;	// delta file
	short dr;	// delta rank
};

struct Vector {
	short  			 c;	// max number of moves
	std::vector<Dir> d;
};

union Nibbles {
	uint8_t b;
	struct {
		uint8_t lo : 4;
		uint8_t hi : 4;
	} f;
};

union Move {
	uint8_t b;
	struct {
		uint8_t action    : 4;
		uint8_t piece     : 6;
		uint8_t target    : 6;
	} f;
};

struct Pos {
	Rank r;
	File f;

	Pos() : r{R1}, f{Fa} {}

	Pos(Rank ra, File fi)
	{
		set(ra,fi);
	}

	void set(Rank ra, File fi) {
		r = ra;
		f = fi;
	}

	uint8_t toByte() {
		return (uint8_t)(r << 3 | f);
	}

	inline Rank rank() { return r; }
	inline File file() { return f; }
};

typedef std::vector<Move>   MoveList;
typedef MoveList::iterator  MoveListItr;
