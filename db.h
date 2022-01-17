#pragma once

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


    PositionPacked& setGameInfo(GameInfo& gi) {
        g = gi.pack();
        return *this;
    }

    // PositionPacked& setBoardBuffer(BoardBuffer& bu) {
    //     b = bu.pack();
    //     return *this;
    // }

    PositionPacked& setMoves(std::vector<Move> m) {
        return *this;
    }
};



#pragma pack()
