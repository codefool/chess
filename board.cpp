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

#include "piece.h"
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

std::vector<Offset> Board::s_ko = {
	{+1,-2}, {+1,+2},
	{+2,-1}, {+2,+1},
	{-2,-1}, {-2,+1},
	{-1,-2}, {-1,+2}
};


Board::Board(bool init)
{
    // set the initial position.
    static PieceType court[] = {
        PT_ROOK, PT_KNIGHT, PT_BISHOP, PT_QUEEN,
        PT_KING, PT_BISHOP, PT_KNIGHT, PT_ROOK
    };

    memset(_b, PT_EMPTY, 64);
    if (init) {
        int f = Fa;
        for( PieceType pt : court) {
            File fi = static_cast<File>(f++);
            place_piece(pt,      SIDE_WHITE, R1, fi);    // white court piece
            place_piece(PT_PAWN, SIDE_WHITE, R2, fi);    // while pawn
            place_piece(PT_PAWN, SIDE_BLACK, R7, fi);    // black pawn
            place_piece(pt,      SIDE_BLACK, R8, fi);    // black court piece
        }
    }
}

PiecePtr Board::place_piece(PieceType t, Side s, Rank r, File f) {
    PiecePtr ptr = Piece::create(t,s);
    ptr->setPos(r,f);
    _p[ptr->getPos().toByte()] = ptr;
    _b[r][f] = ptr->toByte();
    if (PT_KING == t) {
        _k[s] = ptr;
    }
    return ptr;
}

bool Board::in_bounds(short r, short f) {
    return 0 <= f && f <= 7 && 0 <= r && r <= 7;
}

bool Board::in_bounds(Pos pos) {
    return in_bounds(pos.r(), pos.f());
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
            if (!in_bounds(pos)) {
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

// collect all moves for the existing pieces for side onmove
void Board::get_all_moves(Side onmove, MoveList& moves) {
    for( auto p = _p.begin(); p != _p.end(); ++p ) {
        if ( p->second->getSide() == onmove ) {
            get_moves(p->second, moves);
        }
    }
}

void Board::get_moves(PiecePtr p, MoveList& moves) {
    Side onmove = p->getSide();
    PieceType pt = p->getType();
    std::vector<Dir> dirs;

    if (pt == PT_KNIGHT) {
        for(Offset o : s_ko) {
            Pos pos = p->getPos() + o;
            if( !in_bounds(pos))
                continue;
            Move *mov = check_square(p, pos);
            if(mov != nullptr)
                moves.push_back(*mov);
        }
    } else if (pt == PT_PAWN || pt == PT_PAWN_OFF) {
        // pawns are filthy animals ...
        //
        // 1. Pawns can only move one square forward.
        // 2. Pawns on their home square may move two spaces forward.
        // 3. Pawns may capture directly to the UPL or UPR.
        // 4. A pawn on its own fifth rank may capture a neighboring
        //    pawn en passant moving UPL or UPR iif the target pawn
        //    moved forward two squares on its last on move.
        // 5. A pawn that reaches the eighth rank is promoted.
        //
        // Directions are, of course, side dependent.
        Side s = p->getSide();
        bool isBlack = (s == SIDE_BLACK);
        // Case 1: pawns can only move one square forwad.
        Pos ppos  = p->getPos();
        Dir updn  = (isBlack)?DN:UP;
        Dir updnl = (isBlack)?DNL:UPL;
        Dir updnr = (isBlack)?DNR:UPR;
        Pos pos = ppos + s_os[updn];
        if( piece_info(pos) == PT_EMPTY ) {
            // pawn can move forward
            if ( (isBlack && pos.rank() == R1) || (pos.rank() == R8)) {
                // Case 5. A pawn reaching its eighth rank is promoted
                //
                // As we're collecting all possible moves, record four
                // promotions.
                for( auto action : {MV_PROMOTION_QUEEN, MV_PROMOTION_BISHOP, MV_PROMOTION_KNIGHT, MV_PROMOTION_ROOK})
                    moves.push_back(Move(action,  ppos, pos));
            } else {
                moves.push_back(Move(MV_MOVE, p->getPos(), pos));
            }
            // Case 2: Pawns on their home square may move two spaces.
            if ( (isBlack && ppos.rank() == R7) || ppos.rank() == R2) {
                pos += s_os[updn];
                if( piece_info(pos) == PT_EMPTY ) {
                    moves.push_back(Move(MV_MOVE, ppos, pos));
                }
            }
        }
        // Case 3: Pawns may capture directly to the UPL or UPR.
        // see if an opposing piece is UPL or UPR
        dirs.assign({updnl,updnr});
        gather_moves(p, dirs, 1, moves, true);
        // Case 4. A pawn on its own fifth rank may capture a neighboring pawn en passant moving
        // UPL or UPR iif the target pawn moved forward two squares on its last on move.

        // First check if an en passant pawn exists
        // AND the pawn has not moved off its own rank (is not of type PT_PAWN_OFF)
        // AND pawn is on its fifth rank.
        // AND if target pawn is adjacent to this pawn
        if ( _gi.getEnPassantLatch() && pt == PT_PAWN_OFF ) {
            // an en passant candidate exists
            Rank r_pawn = (isBlack) ? R4 : R5;      // where the pawns are
            Rank r_move = (isBlack) ? R3 : R6;      // the space where our pawn moves
            if( ppos.rank() == r_pawn && abs(ppos.f() - _gi.getEnPassantFile()) == 1) {
                // If so, check if the space above the target pawn is empty.
                // If so, then en passant is possible.
                Pos epos(r_move, _gi.getEnPassantFile()); // pos of target square
                if ( piece_info(epos) == PT_EMPTY) {
                    moves.push_back(Move(MV_EN_PASSANT, pos, epos));
                }
            }
        }
    } else {
        short range = (pt == PT_KING) ? 1 : 7;
        // get DIAGS for KING, QUEEN, or BISHOP
        if (pt == PT_KING || pt == PT_QUEEN || pt == PT_BISHOP) {
            dirs.assign({UPL, UPR, DNL, DNR});
            gather_moves(p, dirs, range, moves);
        }
        // get AXES for KING, QUEEN, or ROOK
        if (pt == PT_KING || pt == PT_QUEEN || pt == PT_ROOK) {
            dirs.assign({UP, DN, LFT, RGT});
            gather_moves(p, dirs, range, moves);
        }
    }
}

void Board::gather_moves(PiecePtr p, std::vector<Dir> dirs, int range, MoveList& moves, bool occupied) {
    Side on_move = p->getSide();
    for (auto d : dirs) {
        Pos pos = p->getPos();
        Offset o = s_os[d];
        for( int r = range; r; --r ) {
            pos += o;
            if( !in_bounds(pos) )
                break;
            Move* mov = check_square(p, pos, occupied);
            if (mov == nullptr)
                break;
            moves.push_back(*mov);
        }
    }
}

Move* Board::check_square(PiecePtr p, Pos pos, bool occupied) {
    auto pi = piece_info(pos);

    if (pi == PT_EMPTY)
        // empty square so record move and continue
        return (occupied) ? nullptr : new Move(MV_MOVE, p->getPos(), pos);

    // otherwise, square is not empty.
    // Otherwise, record capture move and leave.
    PieceType pt = static_cast<PieceType> (pi & PIECE_MASK);
    Side      s  = static_cast<Side>     ((pi & SIDE_MASK) == BLACK_MASK);
    if( s == p->getSide()) {
        // If friendly piece, do not record move and leave.
        return nullptr;
    }
    // if opponent piece is king, then record check
    MoveAction ma = ( pt == PT_KING) ? MV_CHECK : MV_CAPTURE;
    return new Move(ma, p->getPos(), pos);
}

// to test for check, we have to travel all rays and knights moves
// from the position of the king in question to see if any square
// contains an opposing piece capable of attacking the king.
//
// For diags finding a bishop or queen (or pawn at range 1 on opponent side.)
// For axes finding a rook or queen
// For kight can only be a knight.
bool Board::test_for_check(PiecePtr king) {
    Pos src = king->getPos();
    Side side = king->getSide();
    std::vector<Dir> dirs = {UP,  DN,LFT,RGT};
    std::vector<PieceType> pts = {PT_ROOK,  PT_QUEEN};
    bool ret = check_ranges( src, dirs, 7, pts, side);
    if (!ret) {
        dirs.assign({UPL,UPR,DNL,DNR});
        pts .assign({PT_BISHOP, PT_QUEEN});
        ret = check_ranges( src, dirs, 7, pts, side);
    }
    if (!ret) {
        pts.assign({PT_KNIGHT});
        for(Offset o : s_ko) {
            Pos pos = src + o;
            if( !in_bounds(pos))
                continue;
            if ( check_piece(piece_info(pos), pts, side) )
                return true;
        }
    }
    if (!ret) {
        // pawns are filthy animals.
        // if we're white, then pawns can be UPL and UPR
        // if we're black, then pawns can be DNL and DNR
        pts.assign({PT_PAWN});
        if(side == SIDE_WHITE)
            dirs.assign({UPL,UPR});
        else
            dirs.assign({DNL,DNR});
        ret = check_ranges(src, dirs, 1, pts, side);
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
    Side      s  = static_cast<Side>     ((pi & SIDE_MASK) == BLACK_MASK);
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
        if (!in_bounds(pos))
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