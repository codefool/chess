// Chess analysis
//
// Copyright (C) 2021 Garyl Hester. All rights reserved.
// github.com/codefool/chess
//
// Released under the GNU General Public Licence Version 3, 29 June 2007
//
#include <iostream>
#include <algorithm>
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

bool Board::inBounds(Pos pos) {
    return 0 <= pos.r() && pos.r() <= 7 && 0 <= pos.f() && pos.f() <= 7;
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
    for (auto d : dir) {
        Pos pos(start);
        Offset o = s_os[d];
        for(int rng = range; rng > 0; rng--) {
            pos += o;
            if (!inBounds(pos)) {
                std::cout << pos.r() << ',' << pos.f() << " out of bounds" << std::endl;
                break;
            }
            // if we're in bounds, then r and f are valid
            // if (validateMove(start, there, onmove))
            //     p.push_back(there);
        }
    }
    return *this;
}

// to test for check, we have to travel all rays and knights moves
// from the position of the king in question to see if any square
// contains an opposing piece capable of attacking the king.
//
// For diags finding a bishop or queen (or pawn at range 1 on opponent side.)
// For axes finding a rook or queen
// For kight can only be a knight.
bool Board::check(Pos& src) {
    std::vector<Dir> dirs = {UP,  DN,LFT,RGT};
    std::vector<PieceType> pts = {PT_ROOK,  PT_QUEEN};
    bool ret = check_ranges( src, dirs, 7, pts, _gi.getOnMove());
    if (!ret) {
        dirs.assign({UPL,UPR,DNL,DNR});
        pts .assign({PT_BISHOP, PT_QUEEN});
        ret = check_ranges( src, dirs, 7, pts, _gi.getOnMove());
    }
    if (!ret) {
        pts.assign({PT_KNIGHT});
        for(Offset o : Knight::_o) {
            Pos pos = src + o;
            if( !inBounds(pos))
                continue;
            if ( check_piece(piece_info(pos), pts, _gi.getOnMove()) )
                return true;
        }
    }
    if (!ret) {
        // pawns are filthy animals.
        // if we're white, then pawns can be UPL and UPR
        // if we're black, then pawns can be DNL and DNR
        pts.assign({PT_PAWN});
        switch(_gi.getOnMove()) {
            case SIDE_WHITE:
                dirs.assign({UPL,UPR});
                break;
            case SIDE_BLACK:
                dirs.assign({DNL,DNR});
                break;
        }
        ret = check_ranges(src, dirs, 1, pts, _gi.getOnMove());
    }

    return ret;
}

// for each direection provided, walk in that direction until a target is encountered, or
// the walker goes out of bounds
bool Board::check_ranges(Pos& src, std::vector<Dir>& dirs, int range, std::vector<PieceType>& pts, Side side) {
    for(Dir d : dirs) {
        uint8_t pi = search_not_empty(src, d, range);
        if (pi == PT_EMPTY)
            continue;
        if (check_piece(pi, pts, side))
            return true;
    }
    return false;
}

bool Board::check_piece(uint8_t pi, std::vector<PieceType>& trg, Side side) {
    // 1. it's not a friendly piece, and
    // 2. it exists in the trg vector.
    PieceType pt = static_cast<PieceType>(pi & PIECE_MASK);
    Side      s  = static_cast<Side>     ((pi & SIDE_MASK) == SIDE_BLACK);
    if (s != side && std::find(trg.begin(), trg.end(), pt) != trg.end())
        return true;
    return false;
}

uint8_t Board::search_not_empty(Pos& start, Dir dir, int range) {
    Offset o = s_os[dir];
    Pos pos(start);
    uint8_t ret;
    while( range-- ) {
        pos += o;
        if (!inBounds(pos))
            break;
        if ((ret = piece_info(pos)) != PT_EMPTY)
            return ret;
    }
    return PT_EMPTY;
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