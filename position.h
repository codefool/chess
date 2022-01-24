#pragma once

#include "constants.h"
#include "db.h"

struct Position {
    GameInfo _g;
    PiecePtr _b[64];

    Position();
    Position(const PositionPacked& p);
    uint32_t unpack(const PositionPacked& p);
    PositionPacked pack();

    void set(int square, PieceType pt, Side s);
    void set(int square, PiecePtr pp);
    PiecePtr get(int square) const;
    bool is_square_empty(int square);
};