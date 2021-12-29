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

std::map<Dir,Offset> Board::s_os = {
	{UP, {+0,+1}},
	{DN, {+0,-1}},
	{LFT,{-1,+0}},
	{RGT,{+1,+0}},
	{UPR,{+1,+1}},
	{UPL,{-1,+1}},
	{DNR,{+1,-1}},
	{DNL,{-1,-1}}
};


Board::Board()
{
    // set the initial position.
    static PieceType court[] = {
        PT_ROOK, PT_KNIGHT, PT_BISHOP, PT_QUEEN,
        PT_KING, PT_BISHOP, PT_KNIGHT, PT_ROOK
    };

    memset(_b, PT_EMPTY, 64);
    int f = Fa;
    for( PieceType pt : court) {
        File fi = static_cast<File>(f++);
        placePiece(pt,      SIDE_WHITE, R1, fi);    // white court piece
        placePiece(PT_PAWN, SIDE_WHITE, R2, fi);    // while pawn
        placePiece(PT_PAWN, SIDE_BLACK, R7, fi);    // black pawn
        placePiece(pt,      SIDE_BLACK, R8, fi);    // black court piece
    }
}

void Board::placePiece(PieceType t, Side s, Rank r, File f) {
    PiecePtr ptr = Piece::create(t,s);
    ptr->setPos(r,f);
    _p[ptr->getPos().toByte()] = ptr;
    _b[r][f] = ptr->toByte();
    if (PT_KING == t) {
        _k[s] = ptr;
    }
}

bool Board::inBounds(short r, short f) {
    return 0 <= f && f <= 7 && 0 <= r && r <= 7;
}

// a property of the physical chessboard is that
// if the odd'ness of the rank and file are the
// same, then the square is black, otherwise it's
// white.
bool Board::isBlackSquare(short r, short f) {
    return (r&1) == (f&1);
}

// return a vector of all the squares starting at the given
// position, and continuing in the provided directions for
// range spaces or until it runs out of bounds.
Board& Board::getSquares(Pos start, std::vector<Dir> dir, int range, std::vector<Pos>& p) {
    uint8_t onmove = (_gi.getOnMove() == SIDE_BLACK) ? BLACK_MASK : 0x00;
    short sr = static_cast<short>(start.rank());
    short sf = static_cast<short>(start.file());
    for (auto d : dir) {
        Offset o = s_os[d];
        short r = sr;
        short f = sf;
        for(int rng = range; rng > 0; rng--) {
            r += o.dr;
            f += o.df;
            if (!inBounds(r,f)) {
                std::cout << r << ',' << f << " out of bounds" << std::endl;
                break;
            }
            // if we're in bounds, then r and f are good
            Pos there(r,f);
            if (validateMove(start, there, onmove))
                p.push_back(there);
        }
    }
    return *this;
}

bool Board::validateMove(Pos& src, Pos& trg, uint8_t mask) {
    uint8_t pi = static_cast<uint8_t>(_b[trg.r][trg.f]);
    if (!pi) {
        std::cout << trg << " empty" << std::endl;
        return true;
    }

    // if square is not empty
    // check if space is occupied by friendly piece
    if ((pi & SIDE_MASK) == mask) {
        std::cout << trg << " occupied by friendly piece" << std::endl;
        return false;
    }
    // square occupied by an opposing piece
    // check if this move places the onmove king in check.}
    return testForCheck(src, trg);
}

bool Board::testForCheck(Pos& src, Pos& trg) {
    // make a copy of the board
    // make the suggested move
    // see if this places the king in check
    //
    // check all diagonals, axes, and knights from the king's
    // position to determine if he's under attack
    static uint8_t  cpy[8][8];
    memcpy(_b, cpy, sizeof(_b));
    uint8_t piece     = cpy[src.r][src.f];
    cpy[trg.r][trg.f] = piece;
    cpy[src.r][src.f] = PT_EMPTY;

    return false;
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