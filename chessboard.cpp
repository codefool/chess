// Chess analysis
//
// Copyright (C) 2021 Garyl Hester. All rights reserved.
// github.com/codefool/chess
//
// Released under the GNU General Public Licence Version 3, 29 June 2007
//
#include <iostream>
// #include <algorithm>
// #include <cstring>
// #include <chrono>
// #include <filesystem>
// #include <fstream>
// #include <inttypes.h>
// #include <map>
// #include <memory>
// #include <mutex>
// #include <initializer_list>
// #include <signal.h>
#include <thread>

// #include <mysqlx/xdevapi.h>

#include "constants.h"
#include "dht.h"
#include "worker.h"

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

const int CLEVEL = 32;
const int CLEVELSUB1 = CLEVEL - 1;
unsigned long long collisions = 0ULL;


int main() {
    std::string workfilepath(WORK_FILE_PATH);
    std::string fspec = workfilepath + "/run_specs.dat";
    load_stats_file(CLEVEL, fspec);
    open_tables(CLEVEL);

    set_stop_handler();

    std::vector<std::thread> threads;

    time_t tstart = time(0);
    std::stringstream ss;
    ss << "base,parent,get,put,coll,mov/c/n1,move,unres,zcol";
#ifdef ENFORCE_14F_50_MOVE_RULE
    ss << ",dis50,draw50";
#endif
    ss << ",pawn,resolved,FEN" << std::endl;

    std::cout << ss.str();

    for (int i = 0; i < THREAD_COUNT; i++) {
        threads.push_back(std::thread(worker, CLEVEL));
    }

  // also use this main thread
  // worker(CLEVEL, workfilepath);

    for (int i = 0; i < THREAD_COUNT; i++) {
        threads[i].join();
    }

    save_stats_file(fspec);

    time_t tend = time(0);
    double hang = std::difftime(tend, tstart);

    std::cout << std::asctime(std::localtime(&tstart))
              << std::asctime(std::localtime(&tend))
              << ' ' << hang << "s (" << (hang/60.0) << "m)"
              << std::endl;

	return 0;
}
