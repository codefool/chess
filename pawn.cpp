// Chess analysis
//
// Copyright (C) 2021 Garyl Hester. All rights reserved.
// github.com/codefool/chess
//
// Released under the GNU General Public Licence Version 3, 29 June 2007
//
#include "pieces.h"

// pawns are filthy animals
//
// Movement rules are wack.
// 1. On initial move, can move 1 or 2 spaces
// 2. Can only capture UPL or UPR (for white, opp for black)
// 3. Can en passant capture if:
//    a. pawn is on its 5th rank
//    b. has never left its own file, and
//    c. target pawn moved 2 spaces on prior turn.
// 4. If moved 2 spaces, flag as en passant target.
// 5. Upon reaching its eight rank is promoted.
//
MoveList Pawn::getValidMoves() {
    return MoveList();
}
