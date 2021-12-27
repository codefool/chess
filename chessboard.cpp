// Chess analysis
//
// Copyright (C) 2021 Garyl Hester. All rights reserved.
// github.com/codefool/chess
//
// Released under the GNU General Public Licence Version 3, 29 June 2007
//
#include <iostream>
#include <cstring>
#include <map>
#include <memory>
#include <vector>
#include <initializer_list>

#include "constants.h"
#include "pieces.h"

/*
Maximum possible moves for any given position
K -  8 * 1 =  8
Q - 28 * 1 = 28 (R+B, so can use this in determining poss. moves for Q)
R - 14 * 2 = 56
N -  8 * 2 = 16
B - 13 * 2 = 26
P -  1 * 8 =  8
            142

<position><possible moves up to 141 for side on move>
Encode in a byte. Hi nibble is piece id 0-31, and lo nibble is target square.
If target square is occupied capture is inferred.
xxxx .... .... .... - action
.... xxxx xx.. .... - Source square
.... .... ..xx xxxx - Target square

Action:
0x0000 moves
0x0001 captures
0x0010 castle kingside
0x0011 castle queenside
0x0100 check
0x0101 checkmate
0x0110 en passant
0x0111 unused
0x1000 promotion queen
0x1001 promotion bishop
0x1010 promotion knight
0x1011 promotion rook
0x1100 unused
0x1101 unused
0x1110 unused
0x1111 unused

Pieces are encoded in nibbles of a 32-byte array, as follows:
x... - side 0=white, 1=black
.xxx - piece type
.001 - King
.010 - Queen
.011 - Bishop
.100 - Knight
.101 - Rook
.110 - Pawn
.111 - Pawn not in its own file
0000 - empty square

Each nibble of the position array represents one square on the chessboard. This is
more efficient than encoding location data per-piece, and also allows (easily) the
addition of duplicate piece types through pawn promotion.

xxxx x... .... .... .... .... .... .... = number of active pieces on the board (0..31)
.... .x.. .... .... .... .... .... .... = side on move: 0-white, 1-black
.... ..xx .... .... .... .... .... .... = drawn game reason
.... .... x... .... .... .... .... .... = white castle kingside disabled  (WK or WKR has moved)
.... .... .x.. .... .... .... .... .... = white castle queenside disabled (WK or WQR has moved)
.... .... ..x. .... .... .... .... .... = black castle kingside disabled  (BK or BKR has moved)
.... .... ...x .... .... .... .... .... = black castle queenside disabled (BK or BQR has moved)
.... .... .... x... .... .... .... .... = en passant latch
.... .... .... .xxx .... .... .... .... = pawn on file xxx is subject to en passant
.... .... .... .... x... .... .... .... = drawn game
.... .... .... .... .xx. .... .... .... = 14D reason
.... .... .... .... ...? ???? ???? ???? = unused (zeroes)

It is imperitive that all unused bits - or bits that are out of scope - be set to 0

Which pieces do we need to know if they have moved or not?
- if pawns are on their home square, then they have not moved (since they can only move forward)
- en passant restrictions:
  - the capturing pawn must have advanced exactly three ranks
  - the captured pawn must have moved two squares in one move
  - the en passant capture must be performed on the turn immediately after the pawn being
    captured moves, otherwise it is illegal.

  This is handled through two mechanisms:
   - When a pawn moves off its own file (via capture), its type is changed to PT_PAWN_O.
   - When a pawn advances two spaces on its first move, en_passant_latch is set to 1, and en_passant_file
     records the file where the pawn rests. On the subsequent position(s), the latch and file are reset to
     zero.
   This works because there can be at most one pawn subject to en passant, but multiple opponent pawns that
   could affect en passant on that pawn. If en passant is affected, it must be done on the VERY NEXT MOVE
   by the opponent, hence its not a permanent condition we need to maintain. For a pawn to affect en passant,
   however, if can not have moved off its own file and must have advanced exactly three ranks - but not
   necessarilly on consecutive turns. So, we have to record the fact that a pawn has changed files.

- castle restrictions:
  - king and rook have not moved
  - king is not in check (cannot castle out of check)
  - king does not pass through check
  - no pieces between the king and rook
*/
#pragma pack(1)
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

union GameInfo {
	uint32_t i;
	struct {
		// number of active pieces on the board (0..31)
		uint32_t piece_cnt          :  5;
		uint32_t on_move            :  1; // 0=white on move, 1=black
		// drawn reason
		// 00 - stalemate (14A)
		// 01 - triple of position (14C)
		// 10 - no material (14D)
		// 11 - 50 move rule (14F)
		uint32_t drawn_reason       :  2;
		// Castling is possible only if the participating pieces have not
		// moved (among other rules, but have nothing to do with prior movement.)
		uint32_t wks_castle_disabled:  1;	// WK or WKR has moved
		uint32_t wqs_castle_disabled:  1;	// WK or WQR has moved
		uint32_t bks_castle_disabled:  1;	// BK or BKR has moved
		uint32_t bqs_castle_disabled:  1;	// BK or BQR has moved
		// en passant
		// If set, the pawn that rests on file en_passant_file moved two
		// positions. This signals that a pawn subject to en passant capture
		// exists, and does not mean that there is an opposing pawn that can
		// affect it.
		uint32_t en_passant_latch   :  1;   // pawn subject to en passant
		uint32_t en_passant_file    :  3;   // file number where pawn rests
		//
		uint32_t drawn_game         :  1;	// 1=drawn game (terminal)
		// if drawn reason is 14D, this is the sub-reason:
		// 00 - King v King (14D1)
		// 01 - King v King with Bishop/Knight (14D2)
		// 10 - King with Bishop/Knight v King with Bishop/Knight (14D3)
		uint32_t reason_14d         :  2;
		uint32_t unused             : 13; // for future use
	} f;
};
# pragma pack()

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

std::vector<Dir> King::_d = {UP, DN, LFT,RGT,UPR,UPL,DNR,DNL};
std::vector<Dir> Queen::_d = {UP, DN, LFT,RGT,UPR,UPL,DNR,DNL};
std::vector<Dir> Rook::_d = {UP, DN, LFT,RGT};
std::vector<Offset> Knight::_o = {
	{+1,+2}, {-1,+2},
	{+1,-2}, {-1,-2},
	{-2,+1}, {-2,-1},
	{+1,+2}, {-1,+2}
};
std::vector<Dir> Bishop::_d = {UP, DN, LFT,RGT};

std::shared_ptr<Piece*> Piece::create(PieceType pt, bool s)
{
	switch(pt) {
		case PT_KING:	  return std::make_shared<Piece*>(new King(s));
		case PT_QUEEN:	  return std::make_shared<Piece*>(new Queen(s));
		case PT_BISHOP:   return std::make_shared<Piece*>(new Bishop(s));
		case PT_KNIGHT:   return std::make_shared<Piece*>(new Knight(s));
		case PT_ROOK:     return std::make_shared<Piece*>(new Rook(s));
		case PT_PAWN:
		case PT_PAWN_OFF: return std::make_shared<Piece*>(new Pawn(s,pt==PT_PAWN_OFF));
	}
	return nullptr;
}

class Board {
private:
	uint8_t  _b[8][8];
	// we always need to know where the kings are
	uint8_t  _bk_pos;
	uint8_t  _wk_pos;
	GameInfo _gi;
	std::map<uint8_t,std::shared_ptr<Piece*>> _p;

public:
	Board()
	{
		memset(_b, PT_EMPTY, 64);
		// set the initial position.
		// first set the while/black court pieces
		PieceType court[] = {PT_ROOK, PT_KNIGHT, PT_BISHOP, PT_QUEEN, PT_KING, PT_BISHOP, PT_KNIGHT, PT_ROOK};
		int f = Fa;
		for( PieceType pt : court) {
			File fi = (File)f++;
			placePiece(pt,      false, R1, fi);
			placePiece(PT_PAWN, false, R2, fi);
			placePiece(PT_PAWN, true,  R7, fi);
			placePiece(pt,      true,  R8, fi);
		}
	}

	GameInfo& gi() { return _gi; }

	void placePiece(PieceType t, bool s, Rank r, File f) {
		std::shared_ptr<Piece*> ptr = Piece::create(t,s);
		uint8_t pos = (r << 3) | f;
		_p[pos] = ptr;
	}

	bool inBounds(short f, short r) {
		return Fa <= f && f <= Fh && R1 <= r <= R8;
	}

	// a property of the physical chessboard is that
	// if the odd'ness of the rank and file are the
	// same, then the square is black, otherwise it's
	// white.
	bool isBlackSquare(short r, short f) {
		return (r&1) == (f&1);
	}

	void dump() {
		for(int r = R8; r >= R1; r--) {
			uint8_t rank = r << 3;
			for(int f = Fa; f <= Fh; f++) {
				char c = '.';
				auto itr = _p.find(rank | f);
				if (itr != _p.end()) {
					c = (*itr->second)->toChar();
				}
				std::cout << ' ' << c;
			}
			std::cout << std::endl;
		}
		for( auto it = _p.begin(); it != _p.end(); ++it) {
			if ((*it->second)->is_on_move(_gi.f.on_move))
				std::cout << ' ' << (*it->second)->toChar();
		}
		std::cout << std::endl;
	}
};

class Game {
private:
		GameInfo _g;
};

int main() {
	short x{7};
	for( short y{0}; y < 8; y++) {
		x--;
		std::cout << x << ' ' << std::hex << (x < 8) << std::endl;
	}

	GameInfo g;
	g.i = 0;
	g.f.drawn_game = 1;
	g.f.drawn_reason = 0x02;
	g.f.reason_14d = 0x02;
	g.f.en_passant_latch = 1;
	g.f.en_passant_file = Fg;

	std::cout << std::hex << g.i << std::endl;

	Board b;

	b.dump();

	return 0;
}

// validate that the given square is either empty or
// occupied by an opponent piece.
bool moveIsValid(bool side, File file, Rank rank) {
	return false;
}

// determine side's king is under attack by any opponent
// piece.
bool inCheck(bool side) {
	// we do this by following all paths to see if there
	// exists an opposing piece which can attack on that
	// path.
	return false;
}

void bogus() {
	PieceType typ;

	if (typ == PT_KING) {
		// check one spot in each direction to see if
	} else if (typ == PT_KNIGHT) {

	}




}