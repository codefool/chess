// Chess analysis
//
// Copyright (C) 2021 Garyl Hester. All rights reserved.
// github.com/codefool/chess
//
// Released under the GNU General Public Licence Version 3, 29 June 2007
//
#pragma once
#include <iostream>
#include <map>
#include <memory>
#include <vector>

enum Side {
	SIDE_WHITE = 0,
	SIDE_BLACK = 1
};

enum EndGameReason {
	EGR_NONE               = 0x00,
	//
	EGR_CHECKMATE          = 0x01,
	EGR_14A_STALEMATE      = 0x02,
	EGR_14C_TRIPLE_OF_POS  = 0x03,
	EGR_14D_NO_MATERIAL    = 0x04,
	EGR_14D1_KVK           = 0x05,
	EGR_14D2_KVKWBN        = 0x06,
	EGR_14D3_KWBVKWB       = 0x07,
	EGR_14D4_NO_LGL_MV_CM  = 0x08,
	EGR_14F_50_MOVE_RULE   = 0x09
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

enum EnPassantFile {
	EP_NONE = 0x00,
	EP_FA   = 0x08,
	EP_FB   = 0x09,
	EP_FC   = 0x0A,
	EP_FD   = 0x0B,
	EP_FE   = 0x0C,
	EP_FF   = 0x0D,
	EP_FG   = 0x0E,
	EP_FH   = 0x0F
};

#define EP_HERE_MASK 0x08
#define EP_FILE_MASK 0x07

enum PieceType {
	PT_EMPTY     = 0x00,
	PT_KING      = 0x01,  // 0x9
	PT_QUEEN     = 0x02,  // 0xa
	PT_BISHOP    = 0x03,  // 0xb
	PT_KNIGHT    = 0x04,  // 0xc
	PT_ROOK      = 0x05,  // 0xd
	PT_PAWN      = 0x06,  // on its own file
	PT_PAWN_OFF  = 0x07   // off its own file
};

#define	SIDE_MASK  0x08
#define	BLACK_MASK 0x08
#define	PIECE_MASK 0x07

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

struct Vector {
	short  			 c;	// max number of moves
	std::vector<Dir> d;
};

// the MoveAction is packed to 4 bits, so 0..15
enum MoveAction {
	MV_NONE             = 0,
	MV_MOVE             = 1,
	MV_CASTLE_KINGSIDE  = 2,
	MV_CASTLE_QUEENSIDE = 3,
	MV_EN_PASSANT       = 4,
	// MV_UNUSED = 5,
	// MV_UNUSED = 6,
	// MV_UNUSED = 7,
	MV_PROMOTION_QUEEN  = 8,
	MV_PROMOTION_BISHOP = 9,
	MV_PROMOTION_KNIGHT = 10,
	MV_PROMOTION_ROOK   = 11
	// UNUSED = 12
	// UNUSED = 13
	// UNUSED = 14
	// UNUSED = 15
};

struct Offset {
private:
	short _dr;
	short _df;
public:
	Offset(short delta_r, short delta_f)
	: _dr{delta_r}, _df{delta_f}
	{}
	short dr() const { return _dr; }
	short df() const { return _df; }
};

class Pos {
private:
	short _r;
	short _f;
public:
	Pos();
	Pos(Rank ra, File fi);
	Pos(int ra, int fi);
	Pos(short pos);
	void set(short pos);
	void set(Rank ra, File fi);
	bool in_bounds() const;
	Pos operator+(const Offset& o) const;
	Pos operator+=(const Offset& o);
	bool operator<(const Pos& o) const;
	bool operator<=(const Pos& o) const;
	bool operator==(const Pos& o) const;
	uint8_t toByte() const;
	void fromByte(uint8_t b);
	const short r() const;
	const short f() const;

	const Rank rank() const;
	const File file() const;
	Pos withRank(Rank r);
	Pos withFile(File f);

	friend std::ostream& operator<<(std::ostream& os, const Pos& p);
};

extern const Pos POS_WQR;
extern const Pos POS_WKR;
extern const Pos POS_BQR;
extern const Pos POS_BKR;

#define g_rank(r) static_cast<char>('1' + r)
#define g_file(f) static_cast<char>('a' + f)

class Piece;
typedef std::shared_ptr<Piece> PiecePtr;

class Piece
{
private:
	PieceType _t;
	Side      _s;
	char	  _c;
	Pos 	  _p;

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

	Pos getPos() { return _p; }

	void setPos(Rank r, File f) {
		_p.set(r,f);
	}

	void setPos(Pos p) {
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
	uint8_t toByte() const;

	static PiecePtr create(PieceType pt, Side s);
	static PiecePtr EMPTY;
};


#pragma pack(1)
union MovePacked {
	uint16_t i;
	struct {
		uint16_t action    : 4;
		uint16_t source    : 6;
		uint16_t target    : 6;
	} f;

	MovePacked() : i{0} {}
	MovePacked(uint16_t ii) : i{ii} {}
};

/*
xxxx x... .... .... .... .... .... .... = number of active pieces on the board (0..31)
.... .x.. .... .... .... .... .... .... = side on move: 0-white, 1-black
.... .... ..x. .... .... .... .... .... = en passant latch
.... .... ...x xx.. .... .... .... .... = pawn on file xxx is subject to en passant
.... .... .... ..x. .... .... .... .... = white castle kingside disabled  (WK or WKR has moved)
.... .... .... ...x .... .... .... .... = white castle queenside disabled (WK or WQR has moved)
.... .... .... .... x... .... .... .... = black castle kingside disabled  (BK or BKR has moved)
.... .... .... .... .x.. .... .... .... = black castle queenside disabled (BK or BQR has moved)
.... .... .... .... ..?? ???? ???? ???? = unused (zeroes)

It is imperitive that all unused bits - or bits that are out of scope - be set to 0
*/

union GameInfoPacked {
	uint32_t i;
	struct {
		uint32_t unused            : 15; // for future use
		// en passant
		// If set, the pawn that rests on file en_passant_file moved two
		// positions. This signals that a pawn subject to en passant capture
		// exists, and does not mean that there is an opposing pawn that can
		// affect it.
		uint32_t en_passant_file   :  4; // file number where pawn rests
		// Castling is possible only if the participating pieces have not
		// moved (among other rules, but have nothing to do with prior movement.)
		uint32_t wks_castle_enabled:  1; // neither WK or WKR has moved
		uint32_t wqs_castle_enabled:  1; // neither WK or WQR has moved
		uint32_t bks_castle_enabled:  1; // neither BK or BKR has moved
		uint32_t bqs_castle_enabled:  1; // neither BK or BQR has moved
		//
		// number of active pieces on the board (2..32)
		uint32_t on_move           :  1; // 0=white on move, 1=black
		uint32_t piece_cnt         :  8;
	} f;

	GameInfoPacked();

	GameInfoPacked(uint32_t v)
	: i{v}
	{}

	GameInfoPacked(const GameInfoPacked& o)
	: i{o.i}
	{}

	bool operator==(const GameInfoPacked& o) const {
		return i == o.i;
	}

	bool operator<(const GameInfoPacked& o) const {
		return i < o.i;
	}
};

// how this works
//
// There are 64 squares on the board, so we keep a bitmap where
// each bit represents whether a piece is present or not. The
// index of the bit is the square number, and the cnt of the bit
// is the index into the lo/hi population to identify what the
// piece is.
//
// lo and hi are 64-bit values, mapped into 16 4-bit values that
// identify each piece. Since there can be no more than 32 pieces
// this sufficies for encoding any possible possition.
//
// This is the packed encoding for the initial position:
//
//   uint64_t pop = 0xffff00000000ffff;
//   uint64_t hi  = 0xdcb9abcdeeeeeeee;
//   uint64_t lo  = 0x6666666654312345;
//
// The square numbers (0-63) are represented by the pop value and
// are interpreted left-to-right, so the first bit represent square
// A1 on the board. Another point of confusion is that since there
// can be at most 32 pieces on the board, then there can only be 32
// bits set in this 64 bit map. Also, each piece is encoded as a
// 4-bit value, which is futher packed into two 64-bit values. So,
// as the pop map is traversed, a count of set bits (usually referred
// to as the population of bits) is made, with the ordinal of any
// given bit being the index into the hi/lo packed maps. So even though
// the pop map contains 64 bits, at most its population is 32 bits set.
//
// The lo and hi maps are treated as a single 128-bit value, and more
// specifically as an array of 32 4-bit values that identify each
// specific piece.
//
struct PositionPacked {
    GameInfoPacked gi;
    uint64_t       pop;   // population bitmap
    uint64_t       lo;    // lo 64-bits population info
    uint64_t       hi;    // hi 64-bits population info

    PositionPacked()
    : gi{0}, pop(0), lo(0), hi(0)
    {}

	PositionPacked(const PositionPacked& o)
	: gi{o.gi}, pop(o.pop), lo(o.lo), hi(o.hi)
	{}

    PositionPacked(uint16_t g, uint64_t p, uint64_t l, uint64_t h)
    : gi{g}, pop(p), lo(l), hi(h)
    {}

    bool operator==(const PositionPacked& o) const {
        return pop == o.pop && gi == o.gi && hi == o.hi && lo == o.lo;
    }

    bool operator<(const PositionPacked& o) const {
		if ( pop == o.pop )
		{
			if ( gi.i == o.gi.i )
			{
				if ( hi == o.hi )
				{
					return lo < o.lo;
				}
				return hi < o.hi;
			}
			return gi.i < o.gi.i;
		}
		return pop > o.pop;
    }
};

#pragma pack()

class Move {
private:
	MoveAction  _a;
	Pos         _s;
	Pos         _t;

public:
	Move();
	Move(MoveAction a, Pos from, Pos to);
	void setAction(MoveAction a);
	MoveAction getAction();

	Pos getSource();
	Pos getTarget();

	static Move unpack(MovePacked& p);
	MovePacked pack();

	friend std::ostream& operator<<(std::ostream& os, const Move& p);
};

typedef std::vector<Move>   MoveList;
typedef MoveList::iterator  MoveListItr;

class GameInfo {
private:
	// number of active pieces on the board (1..31)
	short           piece_cnt;
	Side            on_move; // 0=white on move, 1=black
	// en passant
	// If set, the pawn that rests on file en_passant_file moved two
	// positions. This signals that a pawn subject to en passant capture
	// exists, and does not mean that there is an opposing pawn that can
	// affect it.
	EnPassantFile en_passant_file;
	// Castling is possible only if the participating pieces have not
	// moved (among other rules, but have nothing to do with prior movement.)
	bool          wks_castle_enabled;
	bool          wqs_castle_enabled;
	bool          bks_castle_enabled;
	bool          bqs_castle_enabled;
public:
	GameInfo();

	GameInfo(const GameInfo& o)
	{
		unpack(o.pack());
	}

	void init();

	short getPieceCnt() const { return piece_cnt; }
	void setPieceCnt(short cnt) { piece_cnt = cnt; }
	void decPieceCnt() { piece_cnt--; }
	Side getOnMove() const { return on_move; }
	void setOnMove(Side m) { on_move = m; }
	void toggleOnMove() { on_move = (on_move == SIDE_WHITE) ? SIDE_BLACK : SIDE_WHITE; }
	bool anyCastlePossible() const
	{
		return isWksCastleEnabled() || isWqsCastleEnabled() || isBksCastleEnabled() || isBqsCastleEnabled();
	}
	bool isWksCastleEnabled() const { return wks_castle_enabled; }
	void setWksCastleEnabled(bool s) { wks_castle_enabled = s;}
	bool isWqsCastleEnabled() const { return wqs_castle_enabled; }
	void setWqsCastleEnabled(bool s) { wqs_castle_enabled = s;}
	bool isBksCastleEnabled() const { return bks_castle_enabled; }
	void setBksCastleEnabled(bool s) { bks_castle_enabled = s;}
	bool isBqsCastleEnabled() const { return bqs_castle_enabled; }
	void setBqsCastleEnabled(bool s) { bqs_castle_enabled = s;}
	bool enPassantExists() const { return (en_passant_file & EP_HERE_MASK) != 0;}
	File getEnPassantFile() const { return static_cast<File>(en_passant_file & EP_FILE_MASK); }
	void setEnPassantFile(EnPassantFile ep) { en_passant_file = ep; }
	void setEnPassantFile(File f) {
		setEnPassantFile(static_cast<EnPassantFile>(EP_HERE_MASK | f));
	}

	GameInfo& unpack(const GameInfoPacked& p);
	const GameInfoPacked pack() const;

	bool operator==(const GameInfo& o) const {
		return pack() == o.pack();
	}

	friend std::ostream& operator<<(std::ostream& os, const GameInfo& o);
};

class Position {
private:
    GameInfo _g;
    PiecePtr _b[64];
    Pos      _k[2];

public:
    Position();
    Position(const PositionPacked& p);
    Position(const Position& o);
    uint32_t unpack(const PositionPacked& p);
    PositionPacked pack();
    void init();

	GameInfo& gi() { return _g; }

    void set(Pos pos, PieceType pt, Side s);
    void set(Pos pos, PiecePtr pp);
    PiecePtr get(Pos pos) const;

    Pos get_king_pos(Side side);
    std::vector<PiecePtr> get_pieces(Side side);

    const bool is_square_empty(Pos pos) const;

	std::string fen_string() const;
};

class Board {
private:
	// we always need to know where the kings are
	Position	_p;

public:
	Board(bool init=true);
	Board(const Board& other);
	Board(const PositionPacked& p);

	GameInfo& gi() { return _p.gi(); }

	PiecePtr place_piece(PieceType t, Side s, Rank r, File f);

	void get_all_moves(Side onmove, MoveList& moves);
	void get_moves(PiecePtr p, MoveList& moves);
	void check_castle(Side side, MoveAction ma, MoveList& moves);
	void gather_moves(PiecePtr p, std::vector<Dir> dirs, int range, std::vector<Move>& moves, bool occupied = false);
	Move* check_square(PiecePtr p, Pos pos, bool occupied = false);

	bool test_for_attack(Pos src, Side side);
	bool check_ranges(Pos& src, std::vector<Dir>& dirs, int range, std::vector<PieceType>& pts, Side side);
	bool check_piece(PiecePtr ptr, std::vector<PieceType>& trg, Side side);
	PiecePtr search_not_empty(Pos& start, Dir dir, int range);

	bool validate_move(Move mov, Side side);
	void process_move(Move mov, Side side);
	void move_piece(PiecePtr ptr, Pos pos);
	PositionPacked get_packed() { return _p.pack(); }
	Position& getPosition() { return _p; }

	void dump();
	friend std::ostream& operator<<(std::ostream& os, const Board& b);
};
