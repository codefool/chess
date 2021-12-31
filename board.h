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
	static std::map<Dir,Offset> s_os;

public:
	Board();
	GameInfo& gi() { return _gi; }

	void placePiece(PieceType t, Side s, Rank r, File f);
	bool inBounds(short f, short r);
	bool inBounds(Pos pos);
	Board& getSquares(Pos start, std::vector<Dir> dir, int range, std::vector<Pos>& p);

	bool check(Pos& src);
	bool check_ranges(Pos& src, std::vector<Dir>& dirs, int range, std::vector<PieceType>& pts, Side side);
	bool check_piece(uint8_t pi, std::vector<PieceType>& trg, Side side);
	uint8_t search_not_empty(Pos& start, Dir dir, int range);

	uint8_t piece_info(Pos p) {
		return _b[p.r()][p.f()];
	}

	// a property of the physical chessboard is that
	// if the odd'ness of the rank and file are the
	// same, then the square is black, otherwise it's
	// white.
	bool isBlackSquare(short r, short f);

	void dump();
};
