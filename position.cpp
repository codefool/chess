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

PiecePtr Position::get(Pos pos) const
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

bool Position::is_square_empty(Pos pos) const
{
    return get(pos)->isEmpty();
}
