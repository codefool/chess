#pragma once
#include <cstring>

#include "constants.h"
#include "gameinfo.h"

#pragma pack(1)

struct PositionPacked {
    GameInfoPacked    g;
    BoardBufferPacked b;
    uint8_t           c;
    uint8_t          *m;

    PositionPacked()
    : g{0}, b{0}, c(0), m(nullptr)
    {}


    PositionPacked& setGameInfo(GameInfo& gi);

    // PositionPacked& setBoardBuffer(BoardBuffer& bu);

    PositionPacked& setMoves(std::vector<Move> m);

    bool operator==(const PositionPacked& o) const;
};



#pragma pack()
