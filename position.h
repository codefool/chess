#pragma once

#include <vector>

#include "constants.h"
#include "db.h"

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

    void set(int square, PieceType pt, Side s);
    void set(int square, PiecePtr pp);
    PiecePtr get(int square) const;

    Pos get_king_pos(Side side);
    std::vector<PiecePtr> get_pieces(Side side);

    bool is_square_empty(int square);
};