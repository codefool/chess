#pragma once
#include <cstring>

#include "constants.h"
#include "gameinfo.h"
#include "piece.h"

#pragma pack(1)


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

    PositionPacked(uint64_t p, uint64_t l, uint64_t h)
    : gi{0}, pop(p), lo(l), hi(h)
    {}

    bool operator==(const PositionPacked& o) const {
        return gi == o.gi && hi == o.hi && lo == o.lo && pop == o.pop;
    }
    bool operator<(const PositionPacked& o) const {
        return gi < o.gi || pop < o.pop || hi < o.hi || lo < o.lo;
    }
};
#pragma pack()

struct Position {
    GameInfo _g;
    PiecePtr _b[64];

    Position() {}

    Position(const PositionPacked& p)
    {
        unpack(p);
    }

    uint32_t unpack(const PositionPacked& p)
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

    PositionPacked pack()
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
            map |= (static_cast<uint64_t>(ptr->toByte()) << (cnt << 2ULL));
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

    void set(int square, PieceType pt, Side s)
    {
        set(square, Piece::create(pt, s));
    }

    void set(int square, PiecePtr pp)
    {
        _b[square] = pp;
    }

    PiecePtr get(int square) const
    {
        return _b[square];
    }

    bool is_square_empty(int square)
    {
        return get(square) == Piece::EMPTY;
    }
};
