// Chess analysis
//
// Copyright (C) 2021 Garyl Hester. All rights reserved.
// github.com/codefool/chess
//
// Released under the GNU General Public Licence Version 3, 29 June 2007
//
#include <iostream>
// #include <cstring>
// #include <ctime>
// #include <chrono>
// #include <filesystem>
// #include <fstream>
// #include <inttypes.h>
// #include <map>
// #include <memory>
// #include <mutex>
// #include <initializer_list>
// #include <algorithm>
// #include <signal.h>
#include <thread>

// #include <mysqlx/xdevapi.h>

#include "dreid.h"
// #include "dht.h"
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

<position><possible moves up to 141 for side on-move>
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
.... .x.. .... .... .... .... .... .... = side on-move: 0-white, 1-black
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
// unsigned long long collisions = 0ULL;
// int checkmate = 0;
// bool stop = false;


// #pragma pack(1)
// struct PosCycle {
//   MovePacked move;
//   PositionId trg;

//   PosCycle(Move m, PositionId t)
//   : move{m.pack()}, trg{t}
//   {}
// };

// typedef std::vector<PosCycle> PosCycleMap;
// typedef PosCycleMap *PosCycleMapPtr;

// struct PosInfo {
//   PositionId     id;        // unique id for this position
//   PositionId     src;       // the parent of this position
//   MovePacked     move;      // the Move that created in this position
//   short          move_cnt;  // number of valid moves for this position
//   short          distance;  // number of moves from the initial position
//   short          fifty_cnt; // number of moves since last capture or pawn move (50-move rule [14F])
//   EndGameReason  egr;       // end game reason
//   PosCycleMapPtr cycles;    // cycles

//   PosInfo()
//   : id{0}, src{0}, move(Move().pack()), move_cnt{0}, distance{0}, fifty_cnt{0},
//     egr{EGR_NONE}, cycles{nullptr}
//   {}

//   PosInfo(PositionId i, PosInfo s, MovePacked m)
//   : id{i}, src{s.id}, move(m), move_cnt{0},
//     distance{s.distance + 1},
//     fifty_cnt{s.fifty_cnt + 1},
//     egr{EGR_NONE}, cycles{nullptr}
//   {}

//   void add_cycle(Move move, PositionId trg)
//   {
//     if (cycles == nullptr) {
//       cycles = new PosCycleMap();
//     }
//     cycles->push_back(PosCycle(move,trg));
//   }
// };
// #pragma pack()

// typedef std::map<PositionPacked,PosInfo> PosMap;
// typedef PosMap *PosMapPtr;

// PosMapPtr initpos;
// PosMapPtr unresolved;
// PosMapPtr resolved;

// std::mutex mtx_id;
// PositionId g_id_cnt = 0x1fe3ec53;
// uint64_t collisioncnt = 0;
// uint64_t initposcnt = 0;
// uint64_t unresolvedn1cnt = 0;
// uint64_t fiftymovedrawcnt = 0;

// class PositionFile {
// private:
//   std::string   fspec;
//   std::ofstream ofs;
// public:
//   PositionFile(std::string base_name, int level)
//   {
//     std::stringstream ss;
//     ss << workfilepath << level << '/';
//     std::filesystem::create_directories(ss.str());
//     ss << base_name << '_' << level << ".csv";
//     fspec = ss.str();
//     ofs.open(fspec, std::ios_base::app);
//     ofs.flags(std::ios::hex);
//   }

//   ~PositionFile()
//   {
//     ofs.close();
//   }

//   void write(const PositionPacked& pos, const PosInfo& info)
//   {
//     //id    gi   pop    hi     lo     src    mv   dist 50m
//     ofs << info.id << ','
//       << pos.gi.i << ','
//       << pos.pop << ','
//       << pos.hi << ','
//       << pos.lo << ','
//       << info.src << ','
//       << info.move_cnt << ','
//       << info.move.i << ','
//       << info.distance << ','
//       << info.fifty_cnt << ',';

//       if (info.cycles == nullptr) {
//         ofs << 0;
//       } else {
//         ofs << info.cycles->size() << ',';
//         bool second = false;
//         for (auto e : *info.cycles) {
//           if (second)
//             ofs << ',';
//           ofs << e.move.i << ',' << e.trg;
//           second = true;
//         }
//       }
//       ofs << std::endl;
//   }
// };


// PositionId get_id()
// {
//   std::lock_guard<std::mutex> lock(mtx_id);
//   return ++g_id_cnt;
// }

// std::mutex unresolved_mtx;

// void dump_map( PosMapPtr m)
// {
//   std::cout << "Dumping " << m->size() << "entries =====================\n" << std::endl;
//   for( auto e : *m) {
//     Board b(e.first);
//     std::cout << e.second.id << ' ' << b.getPosition().fen_string() << std::endl;
//   }
// }

// void worker(int level)
// {
//   std::cout << std::this_thread::get_id() << " starting level " << level << std::endl;

//   PositionFile f_initpos("init_pos", level);
//   PositionFile f_downlevel("init_pos", level - 1);

//   int loop_cnt{0};
//   while (!stop) {
//     PositionPacked base_pos;
//     PosInfo        base_info;
//     PosMap::iterator itr;
//     { // dummy scope
//       std::lock_guard<std::mutex> lock(unresolved_mtx);
//       itr = unresolved->begin();
//       if (itr != unresolved->end()) {
//         base_pos  = itr->first;
//         base_info = itr->second;
//         unresolved->erase(base_pos);
//       }
//     }

//     if (base_pos.pop == 0) {
//       using namespace std::chrono_literals;
//       std::this_thread::sleep_for(500ms);
//       continue;
//     }

//     Board b(base_pos);

//     MoveList moves;
//     Side s = b.gi().getOnMove();

//     b.get_all_moves(s, moves);
//     int pawn_moves{0};

//     if (moves.size() == 0) {
//       // no moves - so either checkmate or stalemate
//       bool onside_in_check = b.test_for_attack(b.getPosition().get_king_pos(s), s);
//       base_info.egr = (onside_in_check) ? EGR_CHECKMATE : EGR_14A_STALEMATE;
//       checkmate++;
//       std::cout << std::this_thread::get_id() << " checkmate/stalemate:" << b.getPosition().fen_string() << std::endl;
//     } else {
//       base_info.move_cnt = moves.size();

//       for (Move mv : moves) {
//         Board bprime(base_pos);
//         bool isPawnMove = bprime.process_move(mv, bprime.gi().getOnMove());
//         // we need to flip the on-move
//         Position pprime = bprime.getPosition();
//         pprime.gi().toggleOnMove();
//         // std::cout << std::this_thread::get_id() << ' ' << pprime.fen_string() << std::endl;
//         PositionPacked posprime = pprime.pack();
//         PosInfo posinfo(get_id(), base_info, mv.pack());

//         // 50-move rule: drawn game if no pawn move or capture in the last 50 moves.
//         // hence, if this is a pawn move or a capture, reset the counter.
//         if (isPawnMove || mv.getAction() == MV_CAPTURE) {
//           posinfo.fifty_cnt = 0;  // pawn move - reset 50-move counter
//         }

//         if (isPawnMove) {
//           // new initial position
//           initposcnt++;
//           pawn_moves++;
//           // initpos->insert({posprime,posinfo});
//           f_initpos.write(posprime,posinfo);
//         } else if (posinfo.fifty_cnt == 50) {
//           // no pawn move or capture in past fifty moves, so draw the game
//           base_info.egr = EGR_14F_50_MOVE_RULE;
//           fiftymovedrawcnt++;
//         } else if (bprime.gi().getPieceCnt() == level-1) {
//           unresolvedn1cnt++;
//           f_downlevel.write(posprime,posinfo);
//         } else if(bprime.gi().getPieceCnt() == level) {
//           auto itr = resolved->find(posprime);
//           if (itr != resolved->end()) {
//             itr->second.add_cycle(mv, posinfo.id);
//             collisioncnt++;
//           } else {
//             itr = unresolved->find(posprime);
//             if ( itr != unresolved->end() ) {
//               itr->second.add_cycle(mv, posinfo.id);
//               collisioncnt++;
//             } else {
//               unresolved->insert({posprime,posinfo});
//             }
//           }
//         } else {
//           std::cout << "ERROR! too many captures" << std::endl;
//           exit(1);
//         }
//       }

//       resolved->insert({base_pos, base_info});

//       std::cout << /*std::this_thread::get_id() <<*/ " base:" << base_info.id
//                 << " mov/p:" << moves.size() << '/' << pawn_moves
//                 << " src:" << base_info.src << " mov:" << Move::unpack(base_info.move)
//                 << " dist:" << base_info.distance << " d50:" << base_info.fifty_cnt
//                 << " C:" << collisioncnt
//                 << " I:" << initposcnt // << initpos->size()
//                 << " R:" << resolved->size()
//                 << " U:" << unresolved->size()
//                 << " U1:" << unresolvedn1cnt
//                 << " 50:" << fiftymovedrawcnt
//                 << ' ' << b.getPosition().fen_string(base_info.distance)
//                 << std::endl;
//     }
//     // dump_map(initpos);
//     // dump_map(unresolved);
//     // dump_map(resolved);
//     stop = stop || unresolved->size() == 0;
//   }

//   PositionFile f_resolved("resolved", level);
//   for (auto e : *resolved)
//     f_resolved.write(e.first, e.second);
//   std::cout << std::this_thread::get_id() << " stopping" << std::endl;
// }

// void worker_db(int level)
// {
//   std::cout << std::this_thread::get_id() << " starting" << std::endl;
//   std::string url = "root@localhost:33060";
//   DatabaseObject db(url);

//   while (!stop) {
//     PositionRecord pr = db.get_next_unresolved_position(level);
//     if (pr.id == 0) {
//       using namespace std::chrono_literals;
//       std::this_thread::sleep_for(500ms);
//       continue;
//     }

//     PositionPacked base_pos = pr.pp;

//     Board b(base_pos);

//     MoveList moves;
//     Side s = b.gi().getOnMove();

//     b.get_all_moves(s, moves);
//     if (moves.size() == 0) {
//       // no moves - so either checkmate or stalemate
//       bool onside_in_check = b.test_for_attack(b.getPosition().get_king_pos(s), s);
//       EndGameReason egr = (onside_in_check) ? EGR_CHECKMATE : EGR_14A_STALEMATE;
//       db.set_endgame_reason(level, pr.id, egr);
//       checkmate++;
//       std::cout << std::this_thread::get_id() << " checkmate/stalemate:" << b.getPosition().fen_string() << std::endl;
//       pr.pp = b.get_packed();
//     } else {
//       std::cout << std::this_thread::get_id() << " base position:" << b.getPosition().fen_string() << " moves:" << moves.size() << std::endl;
//       db.set_move_count(level, pr.id, moves.size());

//       for (Move mv : moves) {
//         Board bprime(base_pos);
//         bprime.process_move(mv, bprime.gi().getOnMove());
//         // we need to flip the on-move
//         Position pprime = bprime.getPosition();
//         pprime.gi().toggleOnMove();
//         // std::cout << std::this_thread::get_id() << ' ' << pprime.fen_string() << std::endl;
//         PositionPacked posprime = pprime.pack();
//         if (bprime.gi().getPieceCnt() < level) {
//           db.create_position(level - 1, pr.id, mv, posprime);
//         } else {
//           db.create_position(level, pr.id, mv, posprime);
//         }
//       }
//     }
//   }
//   std::cout << std::this_thread::get_id() << " stopping" << std::endl;
// }

// void ctrl_c_handler(int s) {
//   stop = true;
// }

int main() {
  // struct sigaction sigIntHandler;

  // sigIntHandler.sa_handler = ctrl_c_handler;
  // sigemptyset(&sigIntHandler.sa_mask);
  // sigIntHandler.sa_flags = 0;

  // sigaction(SIGINT, &sigIntHandler, NULL);

  // initpos       = new PosMap();
  // unresolved    = new PosMap();
  // resolved      = new PosMap();

  Position pos;
  pos.init();

// 0000000000000007,1,20fc0000,ffff00000100feff,dcb9abcdeeeeeeee,6666666654312345,0,6081,2,0,0

  PositionPacked pp(uint32_t(0x20fc0000),uint64_t(0xffff00000100feff),uint64_t(0xdcb9abcdeeeeeeee),uint64_t(0x6666666654312345));
  PosInfo posinfo;
  posinfo.id = 7;
  posinfo.src = 1;
  posinfo.distance = 2;
  posinfo.move.i = 0x6081;
  // unresolved->insert({pp,posinfo});
  set_global_id_cnt(1000);
  insert_unresolved(pp,posinfo);

  // {
  //   std::string url = "root@localhost:33060";
  //   DatabaseObject db(url);
  //   db.create_position_table(CLEVEL);
  //   db.create_position_table(CLEVELSUB1);
  //   // db.create_moves_table(1);
  //   db.create_position(CLEVEL, 0, Move(), pp);
  // }

  // worker(CLEVEL);

  // time_t start = time(0);

  std::string workfilepath("/mnt/c/tmp/cg/p7/");
  std::thread t0(worker, CLEVEL, workfilepath);
  // std::thread t1(worker, CLEVEL);
  // std::thread t2(worker, CLEVEL);
  // std::thread t3(worker, CLEVEL);
  // std::thread t4(worker, CLEVEL);
  // std::thread t5(worker, CLEVEL);

  // PositionRecord pr = db.get_next_unresolved_position(CLEVEL);

  // while (!stop && pr.id != 0) {
  //   PositionPacked base_pos = pr.pp;

  //   Board b(base_pos);

  //   MoveList moves;
  //   Side s = b.gi().getOnMove();

  //   b.get_all_moves(s, moves);
  //   if (moves.size() == 0) {
  //     // no moves - so either checkmate or stalemate
  //     bool onside_in_check = b.test_for_attack(b.getPosition().get_king_pos(s), s);
  //     if (onside_in_check) {
  //       // checkmate
  //       b.gi().setEndGameReason(EGR_CHECKMATE);
  //     } else {
  //       // stalemate
  //       b.gi().setEndGameReason(EGR_14A_STALEMATE);
  //     }
  //     checkmate++;
  //     std::cout << "checkmate/stalemate:" << b.getPosition().fen_string() << std::endl;
  //     pr.pp = b.get_packed();
  //     db.update_position(CLEVEL, pr);
  //   } else {
  //     std::cout << "base position:" << b.getPosition().fen_string() << std::endl;
  //     db.set_move_count(CLEVEL, pr.id, moves.size());

  //     for (Move mv : moves) {
  //       Board bprime(base_pos);
  //       bprime.process_move(mv, bprime.gi().getOnMove());
  //       // we need to flip the on-move
  //       Position pprime = bprime.getPosition();
  //       pprime.gi().toggleOnMove();
  //       std::cout << pprime.fen_string() << std::endl;
  //       PositionPacked posprime = pprime.pack();
  //       if (bprime.gi().getPieceCnt() == CLEVELSUB1) {
  //         db.create_position(CLEVELSUB1, posprime);
  //       } else {
  //         db.create_position(CLEVEL, posprime);
  //       }
  //     }
  //   }
  //   pr = db.get_next_unresolved_position(CLEVEL);
  // }

  // std::cout << resolved.size() << ' ' << worksubone.size() << ' ' << collisions << ' ' << checkmate << std::endl;

  t0.join();
  // t1.join();
  // t2.join();
  // t3.join();
  // t4.join();
  // t5.join();

  // time_t stop = time(0);

  // double hang = std::difftime(stop, start);

  // std::cout << std::asctime(std::localtime(&start))
  //           << std::asctime(std::localtime(&stop))
  //           << ' ' << hang << '(' << (hang/60.0) << ") secs"
  //           << std::endl;

	return 0;
}
