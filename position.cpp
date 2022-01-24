#include "constants.h"
#include "position.h"

Position::Position() {}

Position::Position(const PositionPacked& p)
{
    unpack(p);
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
        map |= static_cast<uint64_t>(ptr->toByte() << (cnt << 2ULL));
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

void Position::set(int square, PieceType pt, Side s)
{
    set(square, Piece::create(pt, s));
}

void Position::set(int square, PiecePtr pp)
{
    _b[square] = pp;
}

PiecePtr Position::get(int square) const
{
    return _b[square];
}

bool Position::is_square_empty(int square)
{
    return get(square) == Piece::EMPTY;
}
