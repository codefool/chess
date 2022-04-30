#include <cctype>
#include <sstream>
#include "dreid.h"
#include "zobrist.h"

PosInfo::PosInfo()
: id(0),
  parent(0),
  move(Move().pack()),
  move_cnt(0),
  distance(0),
  egr(EGR_NONE)
{}

PosInfo::PosInfo(PositionHash i, PosInfo s, MovePacked m)
: id(i),
  parent(s.id),
  move(m),
  move_cnt(0),
  distance(s.distance),
  egr(EGR_NONE)
{}

bool PosInfo::operator==(const PosInfo& other)
{
    if (id       != other.id
    || parent    != other.parent
    || move.i    != other.move.i
    || move_cnt  != other.move_cnt
    || distance  != other.distance
    || egr       != other.egr
    )
        return false;
    return true;
}

Position::Position() {}

Position::Position(const PositionPacked& p)
{
    unpack(p);
}

Position::Position(const Position& o)
:_g(o._g), _i(o._i)
{
    // deep copy the board buffer so that smart pointers
    // are copied.
    int cnt = 64;
    while(cnt--)
        _b[cnt] = o._b[cnt];
}

Position::Position(const PositionPacked& p, const PosInfo& i)
: _i(i)
{
    unpack(p);
}

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
uint32_t Position::unpack(const PositionPacked& p)
{
    uint32_t bitcnt{0};
    uint64_t pop{p.pop};
    uint8_t  map[32];
    uint64_t buff[2] = {p.hi, p.lo};
    std::memset(map, 0x00, sizeof(map));
    unpack_array((uint8_t*)buff, map, 32);
  	for (short bit{0}; bit < 64; ++bit)
  	{
        if (pop & 1)
        {
            uint8_t   pb = map[bitcnt++];
            PieceType pt = static_cast<PieceType>( pb & PIECE_MASK);
            Side      s  = static_cast<Side>     ((pb & SIDE_MASK) != 0);
            set(bit, pt, s);
        }
        else
        {
            set(bit, Piece::EMPTY);
        }
        pop >>= 1;
  	}
    _g.unpack(p.gi);
    return bitcnt;
}

PositionPacked Position::pack()
{
	PositionPacked pp;
    uint64_t   pop{0};
    uint32_t   bitcnt{0};
    uint8_t    map[32];
    uint64_t   buff[2];
  	for (short bit{0}; bit < 64; ++bit)
    {
        PiecePtr ptr = _b[bit];
        if (!ptr->isEmpty())
        {
            pop |= 0x8000000000000000ULL;    // mark square as occupied
            map[bitcnt++] = ptr->toByte();
        }
        // This bothers me. We need to pack 64 bits, but we only want to do 63 shifts.
        // So we want to shift on every iteration but the last one.
        if ( bit < 63 )
            pop >>= 1;
  	}

    pp.pop = pop;
    pack_array(map, (uint8_t*)buff, bitcnt);
    pp.hi = buff[0];
    pp.lo = buff[1];
    pp.gi  = gi().pack();

    return pp;
}

void Position::init()
{
    // set the initial position.
    static PieceType court[] = {
        PT_ROOK, PT_KNIGHT, PT_BISHOP, PT_QUEEN,
        PT_KING, PT_BISHOP, PT_KNIGHT, PT_ROOK
    };

    for( int i(0); i < 64; i++)
        _b[i] = Piece::EMPTY;

    short f = Fa;
    for( PieceType pt : court ) {
        File fi = static_cast<File>(f++);
        set( Pos(R1, fi), pt,      SIDE_WHITE );    // white court piece
        set( Pos(R2, fi), PT_PAWN, SIDE_WHITE );    // white pawn
        set( Pos(R7, fi), PT_PAWN, SIDE_BLACK );    // black pawn
        set( Pos(R8, fi), pt,      SIDE_BLACK );    // black court piece
    }
    _g.init();
}

void Position::set(Pos pos, PieceType pt, Side s)
{
    set(pos, Piece::create(pt, s));
}

void Position::set(Pos pos, PiecePtr pp)
{
    _b[pos.toByte()] = pp;
    pp->setPos(pos);
    if (pp->getType() == PT_KING)
        _k[pp->getSide()] = pos;
}

PiecePtr Position::get(const Pos pos) const
{
    return _b[pos.toByte()];
}

Pos Position::get_king_pos(Side side) {
    return _k[side];
}

std::vector<PiecePtr> Position::get_pieces(Side side)
{
    std::vector<PiecePtr> ret;
    for( int i(0); i < 64; i++)
    {
        auto p = _b[i];
        if ( p->getSide() == side && !p->isEmpty() )
            ret.push_back( _b[i] );
    }
    return ret;
}

const bool Position::is_square_empty(Pos pos) const
{
    return get(pos)->isEmpty();
}


// in is an octet array of len s
// out is an octet array of len s/2
// pack every two entries into one entry
void Position::pack_array(uint8_t *in, uint8_t *out, size_t s)
{
    // bool     hi = true;
    uint8_t *p  = in;
    uint8_t *q  = out;
    for(int i(0); i < s; i += 2, p += 2)
    {
        *q++ =  ((p[0] << 4) & 0xf0) | (p[1] & 0x0f);

        // if ( hi )
        // {
        //     *q = (*q & 0x0f) | ((*p << 4) & 0xf0);
        // }
        // else
        // {
        //     *q = (*q & 0xf0) | (*p & 0x0f);
        //     q++;
        // }
        // hi = !hi;
    }
}

// in is an octet array of len s
// out is an octet array of len s*2
// upack every entry of in into two entries of out
void Position::unpack_array(uint8_t* in, uint8_t* out, size_t s)
{
    // bool     hi = true;
    uint8_t *p  = in;
    uint8_t *q  = out;
    for(int i(0); i < s; i += 2)
    {
        *q++ = ((*p >> 4) & 0x0f);
        *q++ = ( *p++ & 0x0f );
        // if ( hi )
        // {
        //     *q = ((*p >> 4) & 0x0f);
        // }
        // else
        // {
        //     *q = (*p & 0x0f);
        //     p++;
        // }
        // hi = !hi;
    }
}


// return the position as a Forsyth–Edwards Notation string
//
// A FEN "record" defines a particular game position, all in one text line and using only the
// ASCII character set. A text file with only FEN data records should have the file extension ".fen"
//
// A FEN record contains six fields. The separator between fields is a space. The fields are:
//
// 1. Piece placement (from White's perspective). Each rank is described, starting with rank 8 and
//    ending with rank 1; within each rank, the contents of each square are described from file "a"
//    through file "h". Following the Standard Algebraic Notation (SAN), each piece is identified
//    by a single letter taken from the standard English names (pawn = "P", knight = "N", bishop = "B",
//    rook = "R", queen = "Q" and king = "K"). White pieces are designated using upper-case letters
//    ("PNBRQK") while black pieces use lowercase ("pnbrqk"). Empty squares are noted using digits 1
//    through 8 (the number of empty squares), and "/" separates ranks.
// 2. Active color. "w" means White moves next, "b" means Black moves next.
// 3. Castling availability. If neither side can castle, this is "-". Otherwise, this has one or more
//    letters: "K" (White can castle kingside), "Q" (White can castle queenside), "k" (Black can castle
//    kingside), and/or "q" (Black can castle queenside). A move that temporarily prevents castling does
//    not negate this notation.
// 4. En passant target square in algebraic notation. If there's no en passant target square, this is "-".
//    If a pawn has just made a two-square move, this is the position "behind" the pawn. This is recorded
//    regardless of whether there is a pawn in position to make an en passant capture.
// 5. Halfmove clock: The number of halfmoves since the last capture or pawn advance, used for the
//    fifty-move rule.
// 6. Fullmove number: The number of the full move. It starts at 1, and is incremented after Black's move.
//
std::string Position::fen_string(int move_no) const
{
    std::stringstream ss;
    int emptyCnt{0};
    // field 1 - piece placement from rank 8 to rank 1
    for(int r = R8; r >= R1; r--) {
        uint8_t rank = r << 3;
        for(int f = Fa; f <= Fh; f++) {
            const PiecePtr p = get(rank|f);
            if(p->isEmpty())
                emptyCnt++;
            else
            {
                if(emptyCnt)
                {
                    ss << emptyCnt;
                    emptyCnt = 0;
                }
                ss << p->getPieceGlyph();
            }
        }
        if(emptyCnt)
        {
            ss << emptyCnt;
            emptyCnt = 0;
        }
        if ( r != R1)
            ss << '/';
    }
    // field 2 - active color
    const GameInfo& gi = _g;
    ss << ' ' << ((gi.getOnMove() == SIDE_BLACK) ? 'b' : 'w') << ' ';

    // field 3 - castleing
    if (!gi.hasCastleRights())
        ss << '-';
    else
    {
        if(gi.hasCastleRight(CR_WHITE_KING_SIDE))
            ss << 'K';
        if(gi.hasCastleRight(CR_WHITE_QUEEN_SIDE))
            ss << 'Q';
        if(gi.hasCastleRight(CR_BLACK_KING_SIDE))
            ss << 'k';
        if(gi.hasCastleRight(CR_BLACK_QUEEN_SIDE))
            ss << 'q';
    }
    ss << ' ';
    // field 4 - en passant
    if( !gi.hasEnPassant())
        ss << '-';
    else {
        ss << gi.getEnPassantFile();
    }
    ss << ' ';
    // field 5 - half-move clock
    // field 6 - full-move number
    ss << "0 " << move_no;
    return ss.str();
}

Position Position::parse_fen_string(std::string fen)
{
    Position pos;
    Rank r(R8);
    File f(Fa);
    for (int i(0); i < fen.size(); ++i)
    {
        char c = fen[i];
        if ( c == ' ')
            break;
        if (std::isalpha(c))
        {
            // a specific piece
        } else if (std::isdigit(c))
        {
            // series of EMPTY
        } else if (c == '/')
        {
            // advance to next rank
        }
    }

    return pos;
}
