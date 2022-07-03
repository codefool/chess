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

#include "dreid.h"

namespace dreid {

// these must be specified in the same order as
// enum Dir
Offset offs[] = {
	{+1,+0},  // UP
	{-1,+0},  // DN
	{+0,-1},  // LFT
	{+0,+1},  // RGT
	{+1,+1},  // UPR
	{+1,-1},  // UPL
	{-1,+1},  // DNR
	{-1,-1}   // DNL
};

Offset kn_offs[] = {
	{+1,-2},
    {+1,+2},
	{+2,-1},
    {+2,+1},
	{-2,-1},
    {-2,+1},
	{-1,-2},
    {-1,+2}
};

Board::Board(bool init)
{
    if (init)
    {
        _p.init();
    }
}

Board::Board(const Board& o)
: _p(o._p)
{
}

Board::Board(const Position& p)
: _p(p)
{}

Board::Board(const PositionPacked& p)
{
    _p.unpack(p);
}

GameInfo& Board::gi()
{
    return _p.gi();
}

// collect all moves for the existing pieces for side onmove
//
void Board::get_all_moves(Side side, MoveList& moves) {
    // TODO: if the king is in check, then only moves that
    // get the king out of check are permissable.
    for( auto p : _p.get_pieces(side) )
        get_moves(p, moves);

    // remove any moves that put the king in check
    for ( auto itr = moves.begin(); itr != moves.end();)
    {
        if( !validate_move(*itr, side) )
            itr = moves.erase(itr);
        else
            ++itr;
    }
}

void Board::get_moves(PiecePtr ptr, MoveList& moves) {
    Side side = ptr->getSide();
    bool isBlack = (side == SIDE_BLACK);
    PieceType pt = ptr->getType();
    std::vector<Dir> dirs;

    if (pt == PT_KNIGHT) {
        for(Offset o : kn_offs) {
            Pos pos = ptr->getPos() + o;
            if( !pos.in_bounds() )
                continue;
            MovePtr mov = check_square(ptr, pos);
            if(mov != nullptr)
                moves.push_back(mov);
        }
    } else if (pt == PT_PAWN || pt == PT_PAWN_OFF) {
        // pawns are filthy animals ...
        //
        // 1. Pawns can only move one square forward.
        // 2. Pawns on their home square may move two spaces forward.
        // 3. Pawns may capture directly to the UPL or UPR.
        // 4. A pawn on its own fifth rank may capture a neighboring
        //    pawn en passant moving UPL or UPR iif the target pawn
        //    moved forward two squares on its last on-move.
        // 5. A pawn that reaches the eighth rank is promoted.
        //
        // Directions are, of course, side dependent.
        // Case 1: pawns can only move one square forwad.
        Pos  ppos = ptr->getPos();
        Dir  updn = (isBlack)?DN:UP;
        Rank pnhm = (isBlack)?R7:R2;
        Pos  pos  = ppos + offs[updn];
        if( pos.in_bounds() && _p.is_square_empty(pos) ) {
            // pawn can move forward
            if ( (isBlack && pos.rank() == R1) || (pos.rank() == R8) ) {
                // Case 5. A pawn reaching its eighth rank is promoted
                //
                // As we're collecting all possible moves, record four
                // promotions.
                for( auto action : {MV_PROMOTION_QUEEN, MV_PROMOTION_BISHOP, MV_PROMOTION_KNIGHT, MV_PROMOTION_ROOK})
                    moves.push_back(Move::create(action,  ppos, pos));
            } else {
                moves.push_back(Move::create(MV_MOVE, ptr->getPos(), pos));
            }
            // Case 2: Pawns on their home square may move two spaces.
            if ( ppos.rank() == pnhm ) {
                pos += offs[updn];
                if( pos.in_bounds() && _p.is_square_empty(pos) )
                    moves.push_back(Move::create(MV_MOVE, ppos, pos));
            }
        }
        // Case 3: Pawns may capture directly to the UPL or UPR.
        // see if an opposing piece is UPL or UPR
        if ( isBlack )
        {
            dirs.assign({DNL,DNR});
        }
        else
        {
            dirs.assign({UPL,UPR});
        }
        gather_moves(ptr, dirs, 1, moves, true);
        // Case 4. A pawn on its own fifth rank may capture a neighboring pawn en passant moving
        // UPL or UPR iif the target pawn moved forward two squares on its last on-move.

        // First check if an en passant pawn exists
        // AND the pawn has not moved off its own rank (is not of type PT_PAWN_OFF)
        // AND pawn is on its fifth rank.
        // AND if target pawn is adjacent to this pawn
        if ( pt == PT_PAWN && _p.gi().hasEnPassant() ) {
            // an en passant candidate exists
            Rank r_pawn = (isBlack) ? R4 : R5;      // rank where the pawn is
            Rank r_move = (isBlack) ? R3 : R6;      // rank with  the space where our pawn moves
            // the en passant file is the file that contains the subject pawn
            // if our pawn is one square away (left or right) from the en passant file
            // then we *can* capture that pawn.
            if( ppos.rank() == r_pawn && abs( ppos.f() - _p.gi().getEnPassantFile()) == 1 ) {
                // If so, check if the space above the target pawn is empty.
                // If so, then en passant is possible.
                Pos epos( r_move, _p.gi().getEnPassantFile() ); // pos of target square
                if ( _p.is_square_empty(epos) )
                    moves.push_back( Move::create( MV_EN_PASSANT, ppos, epos ) );
            }
        }
    } else {
        short range = (pt == PT_KING) ? 1 : 7;
        // get DIAGS for KING, QUEEN, or BISHOP
        if ( pt == PT_KING || pt == PT_QUEEN || pt == PT_BISHOP ) {
            dirs.assign({UPL, UPR, DNL, DNR});
            gather_moves(ptr, dirs, range, moves);
        }
        // get AXES for KING, QUEEN, or ROOK
        if ( pt == PT_KING || pt == PT_QUEEN || pt == PT_ROOK ) {
            dirs.assign({UP, DN, LFT, RGT});
            gather_moves(ptr, dirs, range, moves);
        }
        if ( pt == PT_KING && !test_for_attack(_p.get_king_pos(side), side) ) {
            // For casteling to be possible, the king must not have moved,
            // nor the matching rook, the king must not be in check, the
            // spaces between must be vacant AND cannot be under attack.
            if (isBlack) {
                if(_p.gi().hasCastleRight(CR_WHITE_KING_SIDE))
                    check_castle(side, MV_CASTLE_KINGSIDE, moves);

                if(_p.gi().hasCastleRight(CR_WHITE_QUEEN_SIDE))
                    check_castle(side, MV_CASTLE_QUEENSIDE, moves);
            } else {
                if(_p.gi().hasCastleRight(CR_BLACK_KING_SIDE))
                    check_castle(side, MV_CASTLE_KINGSIDE, moves);

                if(_p.gi().hasCastleRight(CR_BLACK_QUEEN_SIDE))
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
    // locations can be hardcoded.
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
        if ( !_p.is_square_empty(p) || test_for_attack(p, side) )
            // square not empty or under attack - can't castle
            return;

    // make additional empty check for queenside castle
    if (isQueenSide && !_p.is_square_empty(Pos(rank,Fb)))
        return;

    // get here if no reason found not to castle.
    moves.push_back(Move::create(ma, Pos(rank,Fe), Pos(rank,(isQueenSide)?Fa:Fh)));
}

// Collect all valid moves for the given piece for the directions provided, but no more than
// range moves. If isPawnCapture is true, then the move is only valid iif the target square is
// occupied by an opposing piece. (this is for pawns. So, while non-pawn pieces can advance
// in a given direction until/unless another piece is encountered, pawns can only advance in
// this concext iif the target square is occupied by an opposing piece.)
void Board::gather_moves(PiecePtr p, DirList& dirs, int range, MoveList& moves, bool isPawnCapture) {
    Side on_move = p->getSide();
    for (auto d : dirs)
    {
        Pos pos = p->getPos();
        Offset o = offs[d];
        int r = range;
        while ( r-- )
        {
            pos += o;
            if( !pos.in_bounds() )
                break; // walked off the edge of the board.

            MovePtr mov = check_square(p, pos, isPawnCapture);
            if (mov == nullptr)
                break; // encountered a friendly piece - walk is over

            moves.push_back(mov);
            if (mov->getAction() == MV_CAPTURE)
                break; // captured an enemy piece - walk is over.
        }
    }
}

MovePtr Board::check_square(PiecePtr p, Pos pos, bool isPawnCapture)
{
    PiecePtr other = _p.get(pos);

    if ( other->isEmpty() )
    {
        // empty square so record move and continue
        // for isPawnCapture, the move is only valid if the space
        // is occupied by an opposing piece. So, if it's empty
        // return nullptr. Otherwise return the move.
        return (isPawnCapture) ? nullptr
                               : Move::create(MV_MOVE, p->getPos(), pos);
    }

    if( other->getSide() == p->getSide())
    {
        // If friendly piece, do not record move and leave.
        return nullptr;
    }

    return Move::create(MV_CAPTURE, p->getPos(), pos);
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
    DirList       dirs = { UP, DN,LFT,RGT };
    PieceTypeList pts  = { PT_ROOK, PT_QUEEN };
    bool ret = check_ranges( src, dirs, 7, pts, side );
    if ( !ret )
    {
        // Step 2. Check diags for bishops or queens.
        dirs.assign({UPL,UPR,DNL,DNR});
        pts .assign({PT_BISHOP, PT_QUEEN});
        ret = check_ranges(src, dirs, 7, pts, side);
    }
    if ( !ret )
    {
        // Step 3. Check for attacking knights
        pts.assign( {PT_KNIGHT} );
        for(Offset o : kn_offs)
        {
            Pos pos = src + o;
            if( !pos.in_bounds() )
                continue;
            if ( check_piece(_p.get(pos), pts, side) )
                return true;
        }
    }
    if ( !ret )
    {
        // Step 4. Check for pawn attacks.
        // if we're white, then pawns can be UPL and UPR
        // if we're black, then pawns can be DNL and DNR
        pts.assign( {PT_PAWN} );
        if( side == SIDE_WHITE )
            dirs.assign( {UPL, UPR} );
        else
            dirs.assign( {DNL, DNR} );
        ret = check_ranges( src, dirs, 1, pts, side );
    }

    return ret;
}

// for each direection provided, walk in that direction until a target is encountered, or
// the walker goes out of bounds
bool Board::check_ranges(Pos& src, DirList& dirs, int range, PieceTypeList& pts, Side side) {
    for(Dir d : dirs)
    {
        PiecePtr pi = search_not_empty(src, d, range);
        if ( pi->isEmpty() )
            continue;
        if (check_piece(pi, pts, side))
            return true;
    }
    return false;
}

bool Board::check_piece(PiecePtr ptr, PieceTypeList& trg, Side side)
{
    // 1. it's not a friendly piece, and
    // 2. it exists in the trg vector.
    if ( ptr->getSide() != side && std::find(trg.begin(), trg.end(), ptr->getType()) != trg.end() )
        return true;
    return false;
}

PiecePtr Board::search_not_empty( Pos& start, Dir dir, int range )
{
    Offset o = offs[ dir ];
    Pos    pos( start );
    while( range-- )
    {
        pos += o;
        if ( !pos.in_bounds() )
            break;
        if ( !_p.is_square_empty( pos ) )
            return _p.get( pos );
    }
    return Piece::EMPTY;
}

void Board::dump()
{
    for( int r = R8; r >= R1; r-- )
    {
        uint8_t rank = r << 3;
        for( int f = Fa; f <= Fh; f++ )
        {
            std::cout << ' ' << _p.get(rank|f)->getPieceGlyph();
        }
        std::cout << std::endl;
    }
}

// validate the provided move by simulating the move,
// then check if the side king is in check.
//
// Return true if the move is valid, false otherwise.
//
bool Board::validate_move(MovePtr mov, Side side)
{
    // by this time we've already validated that the move is technically
    // possible. The only question is whether making the move exposes the
    // king, which can only be done if the board is actually adjusted.
    Pos      src   = mov->getSource();
    Pos      trg   = mov->getTarget();
    PiecePtr piece = _p.get( src );
    PiecePtr other = _p.get( trg );

    // if the actual piece that is moving is the king, then use the
    // new position to test for check. Otherwise, use the saved position.
    Pos king_pos = piece->isType( PT_KING ) ? trg : _p.get_king_pos( side );

    _p.set( trg, piece );         // copy the source piece to the target square
    _p.set( src, Piece::EMPTY );  // vacate the source square

    bool in_check = !test_for_attack( king_pos, side );

    _p.set( src, piece );
    _p.set( trg, other );

    return in_check;
}

void Board::move_piece(PiecePtr ptr, Pos dst)
{
    // first, see if we're capturing a piece. We know this if
    // destination is not empty.
    if ( !_p.is_square_empty( dst ) ) {
        _p.set( dst, Piece::EMPTY );    // remove piece from game
        _p.gi().decPieceCnt();          // update the piece count
    }

    // next, move the piece to the new square and vacate the old one
    Pos org = ptr->getPos();
    _p.set( org, Piece::EMPTY );
    _p.set( dst, ptr );

    // finally, check for key piece moves and update state
    switch(ptr->getType())
    {
    case PT_KING:
        // king moved - castling is no longer possible for that side
        _p.gi().revokeCastleRights( ptr->getSide(), CR_KING_SIDE | CR_QUEEN_SIDE );
        break;
    case PT_ROOK:
        // rook moved - castling to that side is no longer possible
        if ( mapRookMoves.contains( org.toByte() ) )
            _p.gi().revokeCastleRight( mapRookMoves[ org.toByte() ] );
        break;
    case PT_PAWN:
    {
        // in this case, if the source file and target file differ,
        // then the pawn moved off it's home file. Change it's
        // piece type to reflect this
        if (org.file() != dst.file())
        {
            ptr->setType( PT_PAWN_OFF );
        } else if ( org.rank() == ( ( ptr->getSide() ) ? R7 : R2 ) &&
                    dst.rank() == ( ( ptr->getSide() ) ? R5 : R4 )
        )
            // pawn moved from it's home rank forward two spaces
            // this make it subject to en passant
             _p.gi().setEnPassantFile(org.file());
        }
        break;
    }
}

// process the given move on the board.
// return true if this is a pawn move
bool Board::process_move(MovePtr mov, Side side) {
    PiecePtr   ptr = _p.get( mov->getSource() );
    MoveAction ma  = mov->getAction();

    switch( ma )
    {
    case MV_CASTLE_KINGSIDE: {
        // mov.getSource() is the location of the king,
        // mov.getTarget() is the location of the rook
        // Move king to Fg, rook to Ff
        move_piece( ptr, ptr->getPos().withFile(Fg));
        PiecePtr rook = _p.get( mov->getTarget() );
        move_piece( rook, rook->getPos().withFile(Ff));
        }
        break;
    case MV_CASTLE_QUEENSIDE: {
        // mov.getSource() is the location of the king,
        // mov.getTarget() is the location of the rook
        // Move king to Fc, rook to Fd
        move_piece( ptr, ptr->getPos().withFile(Fc));
        PiecePtr rook = _p.get( mov->getTarget().toByte() );
        move_piece( rook, rook->getPos().withFile(Fd));
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
        move_piece( ptr, mov->getTarget());
        break;
    case MV_EN_PASSANT: {
        // move the piece, but remove the pawn "passed by"
        //    a  b  c         a  b  c
        //   +--+--+--+      +--+--+--+
        // 6 |  | P|  |    4 | p|xP|  |
        //   +--/--+--+      +--\--+--+
        // 5 | P|xp|  |    3 |  | p|  |
        //   +--+--+--+      +--+--+--+
        //
        move_piece( ptr, mov->getTarget() );
        // the pawn 'passed by' will be one square toward on-move
        Dir d = (side == SIDE_BLACK) ? UP : DN;
        Pos p = mov->getTarget() + offs[d];
        // remove the piece from the board, but prob. need to record this somewhere.
        _p.set( p, Piece::EMPTY );
        _p.gi().decPieceCnt();
        }
        break;
    }
    return ptr->getType() == PT_PAWN || ptr->getType() == PT_PAWN_OFF;
}

PositionPacked Board::get_packed()
{
    return _p.pack();
}

Position& Board::getPosition()
{
    return _p;
}

} // namespace dreid