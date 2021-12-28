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

#include "pieces.h"
#include "board.h"

Board::Board()
{
    memset(_b, PT_EMPTY, 64);
    // set the initial position.
    // first set the while/black court pieces
    PieceType court[] = {
        PT_ROOK, PT_KNIGHT, PT_BISHOP, PT_QUEEN,
        PT_KING, PT_BISHOP, PT_KNIGHT, PT_ROOK
    };
    int f = Fa;
    for( PieceType pt : court) {
        File fi = static_cast<File>(f++);
        placePiece(pt,      SIDE_WHITE, R1, fi);
        placePiece(PT_PAWN, SIDE_WHITE, R2, fi);
        placePiece(PT_PAWN, SIDE_BLACK, R7, fi);
        placePiece(pt,      SIDE_BLACK, R8, fi);
    }
}

void Board::placePiece(PieceType t, Side s, Rank r, File f) {
    PiecePtr ptr = Piece::create(t,s);
    ptr->setPos(r,f);
    _p[ptr->getPos().toByte()] = ptr;
    if (PT_KING == t) {
        _k[s] = ptr;
    }
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
                c = itr->second->toChar();
            }
            std::cout << ' ' << c;
        }
        std::cout << std::endl;
    }
}