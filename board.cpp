// Chess analysis
//
// Copyright (C) 2021 Garyl Hester. All rights reserved.
// github.com/codefool/chess
//
// Released under the GNU General Public Licence Version 3, 29 June 2007
//
#include <iostream>
#include <cstring>
#include <memory>

#include "board.h"

Board::Board()
{
    memset(_b, PT_EMPTY, 64);
    // set the initial position.
    // first set the while/black court pieces
    PieceType court[] = {PT_ROOK, PT_KNIGHT, PT_BISHOP, PT_QUEEN, PT_KING, PT_BISHOP, PT_KNIGHT, PT_ROOK};
    int f = Fa;
    for( PieceType pt : court) {
        File fi = (File)f++;
        placePiece(pt,      false, R1, fi);
        placePiece(PT_PAWN, false, R2, fi);
        placePiece(PT_PAWN, true,  R7, fi);
        placePiece(pt,      true,  R8, fi);
    }
}

void Board::placePiece(PieceType t, bool s, Rank r, File f) {
    std::shared_ptr<Piece*> ptr = Piece::create(t,s);
    uint8_t pos = (r << 3) | f;
    _p[pos] = ptr;
}

bool Board::inBounds(short f, short r) {
    return Fa <= f && f <= Fh && R1 <= r <= R8;
}

// a property of the physical chessboard is that
// if the odd'ness of the rank and file are the
// same, then the square is black, otherwise it's
// white.
bool Board::isBlackSquare(short r, short f) {
    return (r&1) == (f&1);
}

void Board::dump() {
    for(int r = R8; r >= R1; r--) {
        uint8_t rank = r << 3;
        for(int f = Fa; f <= Fh; f++) {
            char c = '.';
            auto itr = _p.find(rank | f);
            if (itr != _p.end()) {
                c = (*itr->second)->toChar();
            }
            std::cout << ' ' << c;
        }
        std::cout << std::endl;
    }
    for( auto it = _p.begin(); it != _p.end(); ++it) {
        if ((*it->second)->is_on_move(_gi.f.on_move))
            std::cout << ' ' << (*it->second)->toChar();
    }
    std::cout << std::endl;
}