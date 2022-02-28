// Chess analysis
//
// Copyright (C) 2021 Garyl Hester. All rights reserved.
// github.com/codefool/chess
//
// Released under the GNU General Public Licence Version 3, 29 June 2007
//
#include <iostream>
#include <cstring>
#include <map>
#include <memory>
#include <deque>
#include <initializer_list>
#include <algorithm>

#include <mysqlx/xdevapi.h>

#include "constants.h"

/*
Maximum possible moves for any given position
K -  8 * 1 =  8
Q - 28 * 1 = 28 (R+B, so can use this in determining poss. moves for Q)
R - 14 * 2 = 56
N -  8 * 2 = 16
B - 13 * 2 = 26
P -  1 * 8 =  8
            142

<position><possible moves up to 141 for side on move>
Packed in a byte. Hi nibble is piece id 0-31, and lo nibble is target square.
If target square is occupied capture is inferred.
xxxx .... .... .... - action
.... xxxx xx.. .... - Source square
.... .... ..xx xxxx - Target square

Action:
0x0000 moves
0x0001 captures
0x0010 castle kingside
0x0011 castle queenside
0x0100 check
0x0101 checkmate
0x0110 en passant
0x0111 unused
0x1000 promotion queen
0x1001 promotion bishop
0x1010 promotion knight
0x1011 promotion rook
0x1100 unused
0x1101 unused
0x1110 unused
0x1111 unused

Pieces are packed in nibbles of a 32-byte array, as follows:
x... - side 0=white, 1=black
.xxx - piece type
.001 - King
.010 - Queen
.011 - Bishop
.100 - Knight
.101 - Rook
.110 - Pawn
.111 - Pawn not in its own file
0000 - empty square

Each nibble of the position array represents one square on the chessboard. This is
more efficient than encoding location data per-piece, and also allows (easily) the
addition of duplicate piece types through pawn promotion.

xxxx x... .... .... .... .... .... .... = number of active pieces on the board (0..31)
.... .x.. .... .... .... .... .... .... = side on move: 0-white, 1-black
.... ..xx .... .... .... .... .... .... = drawn game reason
.... .... x... .... .... .... .... .... = white castle kingside disabled  (WK or WKR has moved)
.... .... .x.. .... .... .... .... .... = white castle queenside disabled (WK or WQR has moved)
.... .... ..x. .... .... .... .... .... = black castle kingside disabled  (BK or BKR has moved)
.... .... ...x .... .... .... .... .... = black castle queenside disabled (BK or BQR has moved)
.... .... .... x... .... .... .... .... = en passant latch
.... .... .... .xxx .... .... .... .... = pawn on file xxx is subject to en passant
.... .... .... .... x... .... .... .... = drawn game
.... .... .... .... .xx. .... .... .... = 14D reason
.... .... .... .... ...? ???? ???? ???? = unused (zeroes)

It is imperitive that all unused bits - or bits that are out of scope - be set to 0

Which pieces do we need to know if they have moved or not?
- if pawns are on their home square, then they have not moved (since they can only move forward)
- en passant restrictions:
  - the capturing pawn must have advanced exactly three ranks
  - the captured pawn must have moved two squares in one move
  - the en passant capture must be performed on the turn immediately after the pawn being
    captured moves, otherwise it is illegal.

  This is handled through two mechanisms:
   - When a pawn moves off its own file (via capture), its type is changed to PT_PAWN_O.
   - When a pawn advances two spaces on its first move, en_passant_latch is set to 1, and en_passant_file
     records the file where the pawn rests. On the subsequent position(s), the latch and file are reset to
     zero.
   This works because there can be at most one pawn subject to en passant, but multiple opponent pawns that
   could affect en passant on that pawn. If en passant is affected, it must be done on the VERY NEXT MOVE
   by the opponent, hence its not a permanent condition we need to maintain. For a pawn to affect en passant,
   however, if can not have moved off its own file and must have advanced exactly three ranks - but not
   necessarilly on consecutive turns. So, we have to record the fact that a pawn has changed files.

- castle restrictions:
  - king and rook have not moved
  - king is not in check (cannot castle out of check)
  - king does not pass through check
  - no pieces between the king and rook
*/

class Game {
private:
		GameInfo _g;
};

std::deque<PositionPacked> work;
std::deque<PositionPacked> resolved;
std::deque<PositionPacked> worksubone;

const int CLEVEL = 32;
const int CLEVELSUB1 = CLEVEL - 1;
unsigned long long collisions = 0ULL;
int checkmate = 0;

int main() {

  {
      std::string url = "root@localhost:33060";
      mysqlx::Session sess(url);
  }


  Position pos;
  pos.init();
  work.push_back(pos.pack());


  while (!work.empty()) {
    PositionPacked base_pos = work.front();
    work.pop_front();

    Board b(base_pos);

    MoveList moves;
    Side s = b.gi().getOnMove();

    b.get_all_moves(s, moves);
    if (moves.size() == 0) {
      // no moves - so either checkmate or stalemate
      bool onside_in_check = b.test_for_attack(b.getPosition().get_king_pos(s), s);
      if (onside_in_check) {
        // checkmate
        b.gi().setEndGameReason(EGR_CHECKMATE);
      } else {
        // stalemate
        b.gi().setEndGameReason(EGR_14A_STALEMATE);
      }
      checkmate++;
      std::cout << "checkmate/stalemate:" << b.getPosition().fen_string() << std::endl;
      resolved.push_back(b.get_packed());
      continue;
    }

    resolved.push_back(base_pos);
    std::cout << "base position:" << b.getPosition().fen_string() << std::endl;

    for (Move mv : moves) {
      Board bprime(base_pos);
      bprime.process_move(mv, bprime.gi().getOnMove());
      // we need to flip the on-move
      Position pprime = bprime.getPosition();
      pprime.gi().toggleOnMove();
      std::cout << pprime.fen_string() << std::endl;
      PositionPacked posprime = pprime.pack();
      if (bprime.gi().getPieceCnt() == CLEVELSUB1) {
        if (std::find(worksubone.begin(), worksubone.end(), posprime) != worksubone.end())
          continue;
        worksubone.push_back(posprime);
      } else {
        if (std::find(work.begin(), work.end(), posprime) != work.end()) {
          ++collisions;
          continue;
        }
        if (std::find(resolved.begin(), resolved.end(), posprime) != resolved.end()) {
          ++collisions;
          continue;
        }
        work.push_back(posprime);
      }
    }
  }

  std::cout << resolved.size() << ' ' << worksubone.size() << ' ' << collisions << ' ' << checkmate << std::endl;

	return 0;
}
