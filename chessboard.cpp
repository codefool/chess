#include <iostream>
#include <cstring>
#include <map>
#include <vector>
#include <initializer_list>

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
	uint16_t w[2];
	struct {
		// Castling is possible only if the participating pieces have not
		// moved (among other rules, but have nothing to do with prior movement.)
		uint16_t wks_castle_illegal :  1;
		uint16_t wqs_castle_illegal :  1;
		uint16_t bks_castle_illegal :  1;
		uint16_t bqs_castle_illegal :  1;
		uint16_t on_move            :  1; // 0=white on move, 1=black
		uint16_t en_passant_latch   :  1; // pawn subject to en passant
		uint16_t unused_0           :  3;
		uint16_t en_passant_file    :  3; // file number where pawn rests
		//
		uint16_t position_considered:  1;
		uint16_t drawn_game         :  1;	// 1=drawn game (terminal)
		uint16_t drawn_reason       :  2;
		// drawn reason
		// 00 - stalemate (14A)
		// 01 - triple of position (14C)
		// 10 - no material (14D)
		// 11 - 50 move rule (14F)
		uint16_t reason_14d         :  2;
		// if drawn reason is 14D, this is the sub-reason:
		// 00 - King v King (14D1)
		// 01 - King v King with Bishop/Knight (14D2)
		// 10 - King with Bishop/Knight v King with Bishop/Knight (14D3)
		uint16_t unused_1           : 14; // for future use
	} f;
};
# pragma pack()

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

// const uint8_t SIDE_MASK = 0x08;

enum DrawReason {
	R_NONE                = 0x00,
	//
	R_14A_STALEMATE       = 0x00,
	R_14C_TRIPLE_OF_POS   = 0x01,
	R_14D_NO_MATERIAL     = 0x02,
	R_14F_50_MOVE_RULE    = 0x03,
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

enum Dir {
	UP = 0,
	DN,
	LFT,
	RGT,
	UPR,
	UPL,
	DNR,
	DNL,
	END
};

struct Offset {
	short df;	// delta file
	short dr;	// delta rank
};

struct Vector {
	short  			 c;	// max number of moves
	std::vector<Dir> d;
};

std::map<Dir,Offset> offsets = {
	{UP, {+0,+1}},
	{DN, {+0,-1}},
	{LFT,{-1,+0}},
	{RGT,{+1,+0}},
	{UPR,{+1,+1}},
	{UPL,{-1,+1}},
	{DNR,{+1,-1}},
	{DNL,{-1,-1}}
};

std::map<PieceType,Vector> vectors = {
	{PT_KING,  {1, {UP, DN, LFT,RGT,UPR,UPL,DNR,DNL,END}}},
	{PT_QUEEN, {7, {UP, DN, LFT,RGT,UPR,UPL,DNR,DNL,END}}},
	{PT_BISHOP,{7, {UPR,UPL,DNR,DNL,END}}},
	{PT_ROOK,  {7, {UP, DN, LFT,RGT,END}}}
};

// knights are a special case
std::vector<Offset> knight_offsets = {
	{+1,+2}, {-1,+2},
	{+1,-2}, {-1,-2},
	{-2,+1}, {-2,-1},
	{+1,+2}, {-1,+2}
};

// pawns are filthy animals

#define SQUARE(r,f) (u_char)(r | (f<<3))

class Board {
private:
	uint8_t  _b[8][8];
	// we always need to know where the kings are
	uint8_t  _bk_pos;
	uint8_t  _wk_pos;

public:
	Board() {
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

	void placePiece(PieceType t, bool side, Rank r, File f) {
		uint8_t n = t;
		if (PT_EMPTY != t && side)
			n |= SIDE_BLACK;
		_b[r][f] = n;
	}

	PieceType getPiece(File f, Rank r) {
		Nibbles n{_b[r][f]};
		return (PieceType)(n.f.lo);
	}

	bool inBounds(short f, short r) {
		return 0 <= f && f < 8 && 0 <= r < 8;
	}

	void dump() {
		for(int r = R8; r >= R1; r--) {
			for(int f = Fa; f <= Fh; f++) {
				std::cout << r << ',' << f << ' ' << std::hex << (int)_b[r][f] << std::endl;
			}
		}

		for(int r = R8; r >= R1; r--) {
			for(int f = Fa; f <= Fh; f++) {
				char p;
				bool s = _b[r][f] & SIDE_MASK;
				PieceType pt = (PieceType)(_b[r][f] & PIECE_MASK);
				switch(pt) {
					case PT_EMPTY:    p = '.'; break;
					case PT_KING:     p = 'K'; break;
					case PT_QUEEN:    p = 'Q'; break;
					case PT_BISHOP:   p = 'B'; break;
					case PT_KNIGHT:   p = 'N'; break;
					case PT_ROOK:     p = 'R'; break;
					case PT_PAWN:
					case PT_PAWN_OFF: p = 'P'; break;
				}
				if (s)
					p |= 0x20;
				std::cout << ' ' << p;
			}
			std::cout << std::endl;
		}
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

	// for(auto itr : vectors ) {
	// 	std::cout << itr.first << "\n========" << std::endl;
	// 	Vector v = itr.second;
	// 	for(Dir p : v.d) {
	// 		Offset& o = offsets[p];
	// 		short r = Rd;
	// 		short f = F4;
	// 		for(short c = 0; c < v.c; c++) {
	// 			// travel in this direction until we are:
	// 			// 1. out of bounds
	// 			// 2. hit an occupied square
	// 			f += o.df;
	// 			r += o.dr;
	// 			if (!b.inBounds(f, r))
	// 				break;
	// 			std::cout << c << ' ' << f << ',' << r << ' ' << b.inBounds(f, r) << std::endl;
	// 		}
	// 	}
	// }

	// // Knights
	// std::cout << "Knight\n=======" << std::endl;
	// for(auto o : knight_offsets) {
	// 	short f = F4 + o.df;
	// 	short r = Rb + o.dr;
	// 	// if (!b.inBounds(f, r))
	// 	// 	continue;
	// 	std::cout << o.df << ',' << o.dr << ' ' << f << ',' << r << ' ' << b.inBounds(f, r) << std::endl;
	// }

	// Nibbles n{0};
	// n.f.lo = PT_PAWN_OFF;
	// std::cout << std::hex << (int)n.b << std::endl;



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