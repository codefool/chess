// checkEndGame
//
// This routine checks to see if any of the end-of-game conditions exist:
// EGR_NONE              - no condition exists - continue game
// EGR_CHECKMATE         - one king is in check and either
//                         - cannot move out of check, nor
//                         - any piece can block check
// EGR_14A_STALEMATE     - no move can be made that doesn't put king in check
// EGR_14C_TRIPLE_OF_POS -
// EGR_14D1_KVK          - only the two kings remain
// EGR_14D2_KVKWBN       - king vs king w/bishop or knight
// EGR_14D3_KWBVKWB      - king w/bishop vs king w/bishop and bishops on same color
// EGR_14D4_NO_LBL_MV_CM - no legal move leading to checkmate
// EGR_14E1_LONE_KING    - opponent only has a lone king
// EGR_14E2_KBOKK        - opponent has only king and bishop or knight, or knight
//                         and knight, and does not have a forced win
// EGR_14E3_KNN          - Opponent has only king and two knights, player has no
//                         pawns, and opponent does not have a forced win.
// EGR_14F_50_MOVE_RULE  - position has not changed in last 50 moves
//
#include "dreid.h"

namespace dreid {

PiecePtr find( PiecePtrList& list, PieceType type )
{
    for (PiecePtr ptr : list)
        if ( ptr->getType() == type )
            return ptr;
    return nullptr;
}


bool check_EGR_14D2_KVKWBN(PiecePtrList& lhs, PiecePtrList& rhs)
{
    if( lhs.size() == 1 && rhs.size() == 2 )
    {
        // possible. lhs must have king, and rhs must have king and one other
        for ( PiecePtr ptr : rhs )
        {
            if ( ptr->getType() == PT_BISHOP || ptr->getType() == PT_KNIGHT )
                return true;
        }
    }
    return false;
}

bool check_EGR_14D3_KWBVKWB(PiecePtrList& lhs, PiecePtrList& rhs)
{
    if ( lhs.size() == 2 && rhs.size() == 2 )
    {
        PiecePtr lbish = find(lhs, PT_BISHOP);
        if (lbish != nullptr)
        {
            PiecePtr rbish = find(lhs, PT_BISHOP);
            if (rbish != nullptr)
                // now see if both bishops are on same color
                return lbish->getPos().color() == rbish->getPos().color();
        }
    }
    return false;
}

bool check_EGR_14E2_KBOKK( PiecePtrList& lhs )
{
    if ( lhs.size() == 2 || lhs.size() == 3 )
    {
        int knight_cnt(0);
        int bishop_cnt(0);
        for (PiecePtr ptr : lhs)
        {
            PieceType pt = ptr->getType();
            if (pt == PT_KNIGHT)
                knight_cnt++;
            else if (pt == PT_BISHOP)
                bishop_cnt++;
            else if (pt != PT_KING)
                return false;
        }
        // king with bishop or knight, or two knights
        return ((knight_cnt == 0 && bishop_cnt == 1 ) ||
                (knight_cnt == 1 && bishop_cnt == 0 ) ||
                 knight_cnt == 2 );
    }
    return false;
}

bool check_EGR_14E3_KNN(PiecePtrList& plyr, PiecePtrList& oppo)
{
    if (oppo.size() == 3)
    {
        // player can have no pawns
        if (find(plyr,PT_PAWN) == nullptr)
            return false;
        // opponent only has kight and two knights
        int knight_cnt(0);
        for (PiecePtr ptr : oppo)
        {
            PieceType pt = ptr->getType();
            if (pt != PT_KNIGHT && pt != PT_KING)
                return false;
        }
        return true;
    }
    return false;
}

EndGameReason checkEndOfGame(Board& board, MoveList& moves, Side player)
{
    Side opponent = (player == SIDE_BLACK) ? SIDE_WHITE : SIDE_BLACK;
    if ( moves.size() == 0 )
    {
        // no moves - so either checkmate or stalemate
        bool onside_in_check = board.test_for_attack(board.getPosition().get_king_pos(player), player);
        if (onside_in_check)
        {
            return EGR_13A_CHECKMATE;
        }
        return EGR_14A_STALEMATE;
    }
    PiecePtrList plyr_pieces = board.getPosition().get_pieces(player);
    PiecePtrList oppo_pieces = board.getPosition().get_pieces(opponent);
    if ( oppo_pieces.size() == 1 )
    {
        if ( plyr_pieces.size() == 1 )
            // EGR_14D1_KVK - only the two kings remain
            return EGR_14D1_KVK;
        // EGR_14E1_LONE_KING - opponent only has a lone king
        return EGR_14E1_LONE_KING;
    }
    if ( check_EGR_14D2_KVKWBN(plyr_pieces, oppo_pieces) ||
         check_EGR_14D2_KVKWBN(oppo_pieces, plyr_pieces) )  return EGR_14D2_KVKWBN;
    if ( check_EGR_14D3_KWBVKWB(plyr_pieces, oppo_pieces) ) return EGR_14D3_KWBVKWB;
    if ( check_EGR_14E2_KBOKK(oppo_pieces) )                return EGR_14E2_KBOKK;
    if ( check_EGR_14E3_KNN(plyr_pieces, oppo_pieces) )     return EGR_14E3_KNN;

    return EGR_NONE;
}

} // namespace dreid