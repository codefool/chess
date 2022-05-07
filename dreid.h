// Chess analysis
//
// Copyright (C) 2021-2022 Garyl Hester. All rights reserved.
// github.com/codefool/chessgen
//
// Released under the GNU General Public Licence Version 3, 29 June 2007
//
#pragma once
#include <iostream>
#include <fstream>
#include <cstring>
#include <map>
#include <memory>
#include <set>
#include <thread>
#include <vector>

#include "config.h"
#include "dht.h"

namespace dreid {

#define BeginDummyScope {
#define EndDummyScope }

typedef uint64_t PositionHash;

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

#define EP_HERE_MASK 0x08
#define EP_FILE_MASK 0x07

enum EnPassantFile {
	EP_NONE = 0x00,
	EP_FA   = Fa | EP_HERE_MASK,
	EP_FB   = Fb | EP_HERE_MASK,
	EP_FC   = Fc | EP_HERE_MASK,
	EP_FD   = Fd | EP_HERE_MASK,
	EP_FE   = Fe | EP_HERE_MASK,
	EP_FF   = Ff | EP_HERE_MASK,
	EP_FG   = Fg | EP_HERE_MASK,
	EP_FH   = Fh | EP_HERE_MASK
};

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

// the MoveAction is packed to 4 bits, so 0..15
enum MoveAction {
	MV_NONE             = 0,
	MV_MOVE             = 1,
	MV_CAPTURE          = 2,
	MV_CASTLE_KINGSIDE  = 3,
	MV_CASTLE_QUEENSIDE = 4,
	MV_EN_PASSANT       = 5,
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

enum CastleRight
{
    CR_WHITE_KING_SIDE  = 0x01,
    CR_WHITE_QUEEN_SIDE = 0x02,
    CR_BLACK_KING_SIDE  = 0x04,
    CR_BLACK_QUEEN_SIDE = 0x08,
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
    static std::map<unsigned char, short>    s_z;

public:
	Piece(PieceType t, Side s);

	bool is_on_move(Side m) const;

	PieceType getType() const;

	void setType(PieceType t);

	const char getPieceGlyph() const;

	Pos getPos() const;

	void setPos(Rank r, File f);

	void setPos(Pos p);

	bool isType(PieceType pt) const;

	bool isEmpty() const;

	const Side getSide() const;

	// pack piece type and side into 4-bit value
	uint8_t toByte() const;

    short get_zob_idx();

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
.... .... .... ..x. .... .... .... .... = white castle kingside enabled  (WK or WKR has not moved)
.... .... .... ...x .... .... .... .... = white castle queenside enabled (WK or WQR has not moved)
.... .... .... .... x... .... .... .... = black castle kingside enabled  (BK or BKR has not moved)
.... .... .... .... .x.. .... .... .... = black castle queenside enabled (BK or BQR has not moved)
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
        uint32_t castle_rights     :  4;
		//
		// number of active pieces on the board (2..32)
		uint32_t on_move           :  1; // 0=white on move, 1=black
		uint32_t piece_cnt         :  8;
	} f;

	GameInfoPacked();
	GameInfoPacked(uint32_t v);
	GameInfoPacked(const GameInfoPacked& o);
	bool operator==(const GameInfoPacked& o) const;
	bool operator!=(const GameInfoPacked& o) const;
	bool operator<(const GameInfoPacked& o) const;
};

struct PositionPacked {
    GameInfoPacked gi;
    uint64_t       pop;   // population bitmap
    uint64_t       lo;    // lo 64-bits population info
    uint64_t       hi;    // hi 64-bits population info

    PositionPacked();
	PositionPacked(const PositionPacked& o);
    PositionPacked(uint32_t g, uint64_t p, uint64_t h, uint64_t l);
    bool operator==(const PositionPacked& o) const;
    bool operator!=(const PositionPacked& o) const;
    bool operator<(const PositionPacked& o) const;
	friend std::ostream& operator<<(std::ostream& os, const PositionPacked& pp);
};

class Move;
typedef std::shared_ptr<Move> MovePtr;
typedef std::vector<MovePtr>  MoveList;
typedef MoveList::iterator    MoveListItr;

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
    static MovePtr create(MoveAction a, Pos from, Pos to);

	Pos getSource();
	Pos getTarget();

	static Move unpack(MovePacked& p);
	MovePacked pack();

	friend std::ostream& operator<<(std::ostream& os, const Move& p);
};

class GameInfo {
public:
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
    uint8_t       castle_rights;
public:
	GameInfo();
	GameInfo(const GameInfo& o);
	void init();
	short getPieceCnt() const;
	void setPieceCnt(short cnt);
	void decPieceCnt();
	Side getOnMove() const;
	void setOnMove(Side m);
	void toggleOnMove();
    uint8_t getCastleRights() const;
	bool hasCastleRights() const;
    bool hasCastleRight(CastleRight which) const;
    void setCastleRight(CastleRight which, bool state);
	bool hasEnPassant() const;
	File getEnPassantFile() const;
	void setEnPassantFile(EnPassantFile ep);
	void setEnPassantFile(File f);

	GameInfo& unpack(const GameInfoPacked& p);
	const GameInfoPacked pack() const;

	bool operator==(const GameInfo& o) const;

	friend std::ostream& operator<<(std::ostream& os, const GameInfo& o);
};

struct PosInfo {
  PositionHash   id;        // unique id for this position
  PositionHash   parent;    // the parent of this position
  MovePacked     move;      // the Move that created in this position
  short          move_cnt;  // number of valid moves for this position
  short          distance;  // number of moves from the initial position
  EndGameReason  egr;       // end game reason

  PosInfo();
  PosInfo(PositionHash i, PosInfo s, MovePacked m);
  bool operator==(const PosInfo& other);
};

typedef std::map<PositionPacked,PosInfo> PosMap;
typedef std::set<PositionPacked>		 PosSet;
typedef PosMap *PosMapPtr;

class Position {
private:
    GameInfo _g;
    PiecePtr _b[64];
    Pos      _k[2];
    PosInfo  _i;

public:
    Position();
    Position(const PositionPacked& p);
    Position(const Position& o);
    Position(const PositionPacked& p, const PosInfo& i);
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

	std::string fen_string(int move_no = 0) const;
	static Position parse_fen_string(std::string fen);

    PositionHash zobrist_hash();

private:
	void pack_array(uint8_t *in, uint8_t *out, size_t s);
	void unpack_array(uint8_t* in, uint8_t* out, size_t s);
};

// We process unresolved positions by distance. Since a position of
// distance n can only generate a position of distance n+1, we only
// have to keep two queues. When the queue at distance n depletes,
// we switch to the n+1 queue which becomes the new n, and the
// old queue becomes the n+1 queue.
struct PositionRec
{
    PositionPacked  pp;
    PosInfo         pi;

    PositionRec() {}
    PositionRec(PositionPacked p, PosInfo i)
    : pp(p), pi(i)
    {}

    PositionRec(Position p, PosInfo i)
    : pp(p.pack()), pi(i)
    {}
};

class Board {
private:
	// we always need to know where the kings are
	PositionRec	_pr;
    Position    _p;

public:
	Board(bool init=true);
	Board(const Board& other);
	Board(const PositionRec& p);

	GameInfo& gi();
    PositionRec& pr();
    Position& pos() { return _p; }

	PiecePtr place_piece(PieceType t, Side s, Rank r, File f);

	void get_all_moves(Side onmove, MoveList& moves);
	void get_moves(PiecePtr p, MoveList& moves);
	void check_castle(Side side, MoveAction ma, MoveList& moves);
	void gather_moves(PiecePtr p, std::vector<Dir> dirs, int range, MoveList& moves, bool occupied = false);
	MovePtr check_square(PiecePtr p, Pos pos, bool occupied = false);

	bool test_for_attack(Pos src, Side side);
	bool check_ranges(Pos& src, std::vector<Dir>& dirs, int range, std::vector<PieceType>& pts, Side side);
	bool check_piece(PiecePtr ptr, std::vector<PieceType>& trg, Side side);
	PiecePtr search_not_empty(Pos& start, Dir dir, int range);

	bool validate_move(MovePtr mov, Side side);
	bool process_move(MovePtr mov, Side side);
	void move_piece(PiecePtr ptr, Pos pos);
	PositionPacked get_packed();
	Position& getPosition();

	void dump();
	friend std::ostream& operator<<(std::ostream& os, const Board& b);
};

#pragma pack(1)
struct PosRef
{
    MovePacked   move;
    PositionHash trg;

    PosRef() {}
    PosRef(Move m, PositionHash t)
    : move{m.pack()}, trg{t}
    {}
};

struct PosRefRec
{
    PositionHash src;
    PositionHash trg;
    MovePacked move;

    PosRefRec() {}
    PosRefRec(PositionHash from, MovePtr m, PositionHash to)
    : src(from), move{m->pack()}, trg(to)
    {}
    PosRefRec(PositionHash from, MovePacked m, PositionHash to)
    : src(from), move{m}, trg(to)
    {}
};

typedef std::vector<PosRef> PosRefMap;
typedef PosRefMap *PosRefMapPtr;

#pragma pack()

} // namespace dreid