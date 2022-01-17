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
        _gi.init();
    }
}

Board::Board(Board& o)
: _gi(o._gi), _p(o._p.begin(), o._p.end())
{
    memcpy(&_k, o._k, sizeof(_k));
    memcpy(_b, o._b, sizeof(BoardBuffer));
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

// collect all moves for the existing pieces for side onmove
void Board::get_all_moves(Side side, MoveList& moves) {
    // TODO: if the king is in check, then only moves that
    // get the king out of check are permissable.
    for( auto p = _p.begin(); p != _p.end(); ++p ) {
        if ( p->second->getSide() == side ) {
            get_moves(p->second, moves);
        }
    }
    // remove any moves that put the king in check
    for ( auto itr = moves.begin(); itr != moves.end();) {
        if( !validate_move(*itr, side) )
            moves.erase(itr);
        else
            ++itr;
    }
}

void Board::get_moves(PiecePtr p, MoveList& moves) {
    Side side = p->getSide();
    bool isBlack = (side == SIDE_BLACK);
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
        if ( pt == PT_PAWN && _gi.enPassantExists() ) {
            // an en passant candidate exists
            Rank r_pawn = (isBlack) ? R4 : R5;      // where the pawns are
            Rank r_move = (isBlack) ? R3 : R6;      // the space where our pawn moves
            if( ppos.rank() == r_pawn && abs(ppos.f() - _gi.getEnPassantFile()) == 1) {
                // If so, check if the space above the target pawn is empty.
                // If so, then en passant is possible.
                Pos epos(r_move, _gi.getEnPassantFile()); // pos of target square
                if ( piece_info(epos) == PT_EMPTY) {
                    moves.push_back(Move(MV_EN_PASSANT, ppos, epos));
                }
            }
        }
    } else {
        short range = (pt == PT_KING) ? 1 : 7;
        // get DIAGS for KING, QUEEN, or BISHOP
        if ( pt == PT_KING || pt == PT_QUEEN || pt == PT_BISHOP ) {
            dirs.assign({UPL, UPR, DNL, DNR});
            gather_moves(p, dirs, range, moves);
        }
        // get AXES for KING, QUEEN, or ROOK
        if ( pt == PT_KING || pt == PT_QUEEN || pt == PT_ROOK ) {
            dirs.assign({UP, DN, LFT, RGT});
            gather_moves(p, dirs, range, moves);
        }
        if ( pt == PT_KING && !test_for_attack(_k[side]->getPos(), side)) {
            // For casteling to be possible, the king must not have moved,
            // nor the matching rook, the king must not be in check, the
            // spaces between must be vacant AND cannot be under attack.
            if (isBlack) {
                if(_gi.isBksCastleEnabled())
                    check_castle(side, MV_CASTLE_KINGSIDE, moves);

                if(_gi.isBqsCastleEnabled())
                    check_castle(side, MV_CASTLE_QUEENSIDE, moves);
            } else {
                if(_gi.isWksCastleEnabled())
                    check_castle(side, MV_CASTLE_KINGSIDE, moves);

                if(_gi.isWqsCastleEnabled())
                    check_castle(side, MV_CASTLE_QUEENSIDE, moves);
            }
        }
    }
}

void Board::check_castle(Side side, MoveAction ma, MoveList& moves) {
    // get here if neither the king nor the rook have moved.
    // 1. The squares between the king and the rook have to be empty [8A4b],
    // 2. The king cannot be in check [8A4a], and
    // 3. The king cannot move over check [8A4a].
    // So, check that the spaces between the king and rook (excluded)
    // are PT_EMPTY.
    //
    // Also, check the first two squares from the king toward the rook are
    // not under attack.
    //                queenside    kingside
    //                R  N  B  Q  K  B  N  R
    //                      <-->     <-->       check for attack
    //                   <----->     <-->       check for empty
    //
    // This logic is rather straighforward, though rather torturned, because
    // the positions of the pieces are defined by the Rules, and hence
    // generating locations can be hardcoded.
    //
    // We assume that the king is not already in check.
    //
    Rank rank = (side == SIDE_WHITE) ? R1 : R8;
    bool isQueenSide = (ma == MV_CASTLE_QUEENSIDE);
    std::vector<Pos> emptyCheck;
    if (isQueenSide) {
        emptyCheck.assign({Pos(rank, Fc), Pos(rank, Fd)});
    } else {
        emptyCheck.assign({Pos(rank, Ff), Pos(rank, Fg)});
    }

    for (auto p : emptyCheck)
        if (piece_info(p) != PT_EMPTY || test_for_attack(p, side))
            // square not empty or under attack - can't castle
            return;

    // make additional empty check for queenside castle
    if (isQueenSide && piece_info(Pos(rank,Fb)) != PT_EMPTY)
        return;

    // get here if no reason found not to castle.
    moves.push_back(Move(ma, Pos(rank,Fe), Pos(rank,(isQueenSide)?Fa:Fh)));
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
// For knight can only be a knight.
bool Board::test_for_attack(Pos src, Side side) {
    // Step 1. Check axes for rooks or queens.
    std::vector<Dir> dirs = {UP,  DN,LFT,RGT};
    std::vector<PieceType> pts = {PT_ROOK,  PT_QUEEN};
    bool ret = check_ranges( src, dirs, 7, pts, side);
    if (!ret) {
        // Step 2. Check diags for bishops or queens.
        dirs.assign({UPL,UPR,DNL,DNR});
        pts .assign({PT_BISHOP, PT_QUEEN});
        ret = check_ranges(src, dirs, 7, pts, side);
    }
    if (!ret) {
        // Step 3. Check for attacking knights
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
        // Step 4. Check for pawn attacks.
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
                c = itr->second->getPieceGlyph();
            }
            std::cout << ' ' << c;
        }
        std::cout << std::endl;
    }
}

// validate the provided move by simulating the move,
// then check if the side king is in check.
//
// Return true if the move is valid, false otherwise.
//
bool Board::validate_move(Move mov, Side side) {
    // I considered employing a do-undo mechanism for this, but
    // it's only 64 freakin' bytes, and the code would be much larger.
    BoardBuffer bup;
    memcpy(bup, _b, sizeof(BoardBuffer));

    // by this time we've already validated that the move is technically
    // possible. The only question is whether making the move exposes the
    // king, which can only be done if the board is actually adjusted.
    Pos src = mov.getSource();
    Pos trg = mov.getTarget();
    uint8_t piece = piece_info(src);
    // if the actual piece that is moving is the king, then use the
    // new position to test for check. Otherwise, use the saved position.
    Pos king_pos = ( (piece & PIECE_MASK) == PT_KING) ? trg : _k[side]->getPos();

    _b[trg.r()][trg.f()] = _b[src.r()][src.f()];  // copy the source piece to the target square
    _b[src.r()][src.f()] = PT_EMPTY;                       // vacate the source square

    bool in_check = !test_for_attack( king_pos, side );

    memcpy(_b, bup, sizeof(BoardBuffer));

    return in_check;
}

void Board::move_piece(PiecePtr ptr, Pos dst) {
    // first, see if we're capturing a piece. We know this if
    // destination is not empty.
    uint8_t  pi = piece_info(dst);
    if (pi != PT_EMPTY) {
        _p.erase(dst.toByte());     // remove piece from game
        _gi.setPieceCnt(_p.size()); // update the piece count
    }
    // next, move the piece to the new square and vacate the old one
    pi = piece_info(ptr->getPos());
    Pos org = ptr->getPos();
    set_piece_info( org, PT_EMPTY );
    set_piece_info( dst, pi );
    ptr->setPos(dst);
    _p.erase(org.toByte());
    _p[dst.toByte()] = ptr;

    // finally, check for key piece moves and update state
    switch(ptr->getType())
    {
    case PT_KING:
        // king moved - casteling is no longer possible
        if (ptr->getSide() == SIDE_WHITE) {
            _gi.setWksCastleEnabled(false);
            _gi.setWqsCastleEnabled(false);
        } else {
            _gi.setBksCastleEnabled(false);
            _gi.setBqsCastleEnabled(false);
        }
        break;
    case PT_ROOK:
        // rook moved - casteling to that side is no longer possible
        if (ptr->getSide() == SIDE_WHITE) {
            if( org == POS_WQR )
                _gi.setWqsCastleEnabled(false);
            else if( org == POS_WKR)
                _gi.setWksCastleEnabled(false);
        } else if( org == POS_BQR)
            _gi.setBqsCastleEnabled(false);
        else if( org == POS_BKR)
            _gi.setBksCastleEnabled(false);
        break;
    case PT_PAWN: {
        // in this case, if the source file and target file differ,
        // then the pawn moved off it's home file. Change it's
        // piece type to reflect this
        if (org.file() != dst.file()) {
            ptr->setType(PT_PAWN_OFF);
        } else if ( org.rank() == ((ptr->getSide())?R7:R2) &&
                    dst.rank() == ((ptr->getSide())?R5:R4)
        )
            // pawn moved from it's home rank forward two spaces
             _gi.setEnPassantFile(org.file());
        }
        break;
    }
}

void Board::process_move(Move mov, Side side) {
    // Board *ret = new Board(*this);
    PiecePtr   ptr = _p[mov.getSource().toByte()];
    MoveAction ma  = mov.getAction();

    switch(mov.getAction())
    {
    case MV_CASTLE_KINGSIDE: {
        // mov.getSource() is the location of the king,
        // mov.getTarget() is the location of the rook
        // Move king to file g, rook to file f
        move_piece( ptr, Pos(ptr->getPos().rank(), Fg));
        PiecePtr rook = _p[mov.getTarget().toByte()];
        move_piece( rook, Pos(rook->getPos().rank(), Ff));
        }
        break;
    case MV_CASTLE_QUEENSIDE: {
        // mov.getSource() is the location of the king,
        // mov.getTarget() is the location of the rook
        // Move king to file c, rook to file d
        move_piece( ptr, Pos(ptr->getPos().rank(), Fc));
        PiecePtr rook = _p[mov.getTarget().toByte()];
        move_piece( rook, Pos(rook->getPos().rank(), Fd));
        }
        break;
    case MV_PROMOTION_QUEEN:
    case MV_PROMOTION_BISHOP:
    case MV_PROMOTION_KNIGHT:
    case MV_PROMOTION_ROOK: {
        PieceType newType = static_cast<PieceType>(ma - MV_PROMOTION_QUEEN + PT_QUEEN);
        ptr->setType(newType);
        }
        [[fallthrough]];
    case MV_MOVE:
    case MV_CAPTURE:
    case MV_CHECK:
    case MV_CHECKMATE:
        move_piece( ptr, mov.getTarget());
        break;
    case MV_EN_PASSANT: {
        // move the piece, but remove the pawn "passed by"
        //    a  b  c         a  b  c
        //   +--+--+--+      +--+--+--+
        // 5 |  | P|  |    4 | p| P|  |
        //   +--/--+--+      +--\--+--+
        // 4 | P| p|  |    3 |  | p|  |
        //   +--+--+--+      +--+--+--+
        //
        move_piece( ptr, mov.getTarget());
        // the pawn 'passed by' will be one square toward on side
        Dir d = (side == SIDE_BLACK) ? UP : DN;
        Pos p = mov.getTarget() + s_os[d];
        // remove the piece from the board, but prob. need to record this somewhere.
        set_piece_info(p, PT_EMPTY);
        }
        break;
    }
}

void packBoardBuffer(const BoardBuffer& b, BoardBufferPacked ppb) {
	uint8_t *p = ppb;
	for(int i = 0; i < 8; ++i) {
		for(int j = 0; j < 8; j+=2) {
			Nibbles n(b[i][j+1], b[i][j]);
			*p++ = n.b;
		}
	}
}

void unpackBoardBuffer(const BoardBufferPacked ppb, BoardBuffer& b) {
	const uint8_t *p = ppb;
	for(int i = 0; i < 8; ++i) {
		for(int j = 0; j < 8; j+=2) {
            Nibbles n(*p++);
            b[i][j+1] = n.f.hi;
            b[i][j]   = n.f.lo;
		}
	}
}

std::ostream& operator<<(std::ostream& os, const Board& b) {
	// the purpose here is to write the game and board information as a series of hex digits
    // first the gameinfo
    BoardBufferPacked ppb;
    packBoardBuffer(b._b, ppb);

    os << std::hex;
    os.fill('0');
    os.width(8);
    os << unsigned(b._gi.pack_c().i) << ' ';

    uint8_t *p = ppb;
    for (int i = 0; i < 32; i++, ++p) {
        os.fill('0');
        os.width(2);
        os << +(*p) << ' ';
    }
    os.flush();
	return os;
}
