#include <sstream>
#include "constants.h"


Position::Position() {}

Position::Position(const PositionPacked& p)
{
    unpack(p);
}

Position::Position(const Position& o)
:_g{o._g}
{
    // deep copy the board buffer so that smart pointers
    // are copied.
    int cnt = 64;
    while(cnt--)
        _b[cnt] = o._b[cnt];
}

uint32_t Position::unpack(const PositionPacked& p)
{
    uint32_t bitcnt(0);
    uint32_t mapidx(0);
    uint64_t map(p.lo);
    for(uint64_t square = 0; square < 64; ++square)
    {
        if (p.pop & (1ULL<<square))
        {
            // determine which map the value should reside
            uint8_t   pb  = static_cast<uint8_t>  (((map >> (mapidx << 2)) & 0xfULL));
            PieceType pt  = static_cast<PieceType>(pb & PIECE_MASK);
            Side      s   = static_cast<Side>     ((pb & SIDE_MASK) != 0);
            set(square, pt, s);
            bitcnt++;
            mapidx++;
            if (bitcnt == 16) {
                map = p.hi;
                mapidx = 0;
            }
        } else {
            set(square, Piece::EMPTY);
        }
    }
    _g.unpack(p.gi);
    return bitcnt;
}

PositionPacked Position::pack()
{
    PositionPacked pp;
    uint64_t pop{0};
    uint64_t map{0};
    uint64_t cnt{0};
    uint32_t bitcnt{0};

    for (int bit(0); bit < 64; bit++) {
        PiecePtr ptr = _b[bit];
        if (ptr->isEmpty())
            continue;
        uint64_t mask = static_cast<uint64_t>(ptr->toByte());
        map |= static_cast<uint64_t>(mask << (cnt << 2ULL));
        pop |= (1ULL << bit);
        cnt++;
        bitcnt++;
        if (bitcnt == 16) {
            pp.lo = map;
            map = 0;
            cnt = 0;
        }
    }

    pp.pop = pop;
    if (bitcnt > 15)
        pp.hi = map;

    pp.gi = gi().pack();
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
    set(pos.toByte(), Piece::create(pt, s));
}

void Position::set(Pos pos, PiecePtr pp)
{
    _b[pos.toByte()] = pp;
    pp->setPos(pos);
    if (pp->getType() == PT_KING)
        _k[pp->getSide()] = pos;
}

const PiecePtr& Position::get(const Pos pos) const
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

std::string Position::fen_string() const
{
    std::stringstream ss;
    int emptyCnt{0};
    // field 1 - piece placement from rank 8 to rank 1
    for(int r = R8; r >= R1; r--) {
        uint8_t rank = r << 3;
        for(int f = Fa; f <= Fh; f++) {
            const PiecePtr& p = get(rank|f);
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
    if (!gi.anyCastlePossible())
        ss << '-';
    else
    {
        if(gi.isWksCastleEnabled())
            ss << 'K';
        if(gi.isWqsCastleEnabled())
            ss << 'Q';
        if(gi.isBksCastleEnabled())
            ss << 'k';
        if(gi.isBqsCastleEnabled())
            ss << 'q';
    }
    ss << ' ';
    // field 4 - en passant
    if( !gi.enPassantExists())
        ss << '-';
    else {
        ss << gi.getEnPassantFile();
    }
    ss << ' ';
    // field 5 - half-move clock
    // field 6 - full-move number
    ss << "0 0";
    return ss.str();
}

