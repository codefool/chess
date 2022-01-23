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
struct PositionPacked {
    GameInfoPacked gi;
    uint64_t       pop;   // population bitmap
    uint64_t       lo;    // lo 64-bits population info
    uint64_t       hi;    // hi 64-bits population info

    PositionPacked()
    : gi{0}, pop(0), lo(0), hi(0)
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

    void unpack(const PositionPacked& p)
    {
        int bitcnt = unpack(p.pop, p.lo,  0, 32)
                   + unpack(p.pop, p.hi, 32, 64);
    }

    int unpack(uint64_t pop, uint64_t map, int startbit, int endbit)
    {
        int bitcnt(0);
        for(int bit=startbit; bit < endbit; ++bit)
        {
            if (pop & (1<<bit))
            {
                uint8_t   pb = static_cast<uint8_t>(((map >> (bitcnt << 2)) & 0x0f));
                PieceType pt = static_cast<PieceType>(pb & PIECE_MASK);
                Side      s  = static_cast<Side>     (pb & SIDE_MASK);
                set(bit, pt, s);
                bitcnt++;
            } else {
                set(bit, Piece::EMPTY);
            }
        }
        return bitcnt;
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
