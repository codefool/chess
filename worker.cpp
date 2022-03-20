#include <iostream>
#include <filesystem>
#include <fstream>
#include <mutex>
#include <signal.h>
#include <sstream>
#include <thread>

#include "worker.h"


PosInfo::PosInfo()
: id{0}, src{0}, move(Move().pack()), move_cnt{0}, distance{0}, fifty_cnt{0},
egr{EGR_NONE}, cycles{nullptr}
{}

PosInfo::PosInfo(PositionId i, PosInfo s, MovePacked m)
: id{i}, src{s.id}, move(m), move_cnt{0},
    distance{s.distance + 1},
    fifty_cnt{s.fifty_cnt + 1},
    egr{EGR_NONE}, cycles{nullptr}
{}

void PosInfo::add_cycle(Move move, PositionId trg)
{
    if (cycles == nullptr) {
        cycles = new PosCycleMap();
    }
    cycles->push_back(PosCycle(move,trg));
}

PositionFile::PositionFile(std::string base_path, std::string base_name, int level)
{
    std::stringstream ss;
    ss << base_path << level << '/';
    std::filesystem::create_directories(ss.str());
    ss << base_name << '_' << level << ".csv";
    fspec = ss.str();
    ofs.open(fspec, std::ios_base::app);
    ofs.flags(std::ios::hex);
    ofs.fill('0');
}

PositionFile::~PositionFile()
{
    ofs.close();
}

void PositionFile::write(const PositionPacked& pos, const PosInfo& info)
{
    //id    gi   pop    hi     lo     src    mv   dist 50m
    ofs << info.id << ','
        << pos.gi.i << ',';
    auto ow = ofs.width(8);
    ofs << pos.pop << ',';
    ofs.width(16);
    ofs << pos.hi << ','
        << pos.lo << ','
        << info.src << ',';
    ofs.width(ow);
    ofs << info.move_cnt << ','
        << info.move.i << ','
        << info.distance << ','
        << info.fifty_cnt << ',';

    if (info.cycles == nullptr) {
        ofs << 0;
    } else {
        ofs << info.cycles->size() << ',';
        bool second = false;
        for (auto e : *info.cycles) {
            if (second)
            ofs << ',';
            ofs << e.move.i << ',' << e.trg;
            second = true;
        }
    }
    ofs << std::endl;
}

std::mutex mtx_id;
PositionId g_id_cnt = 0;

PositionId get_id()
{
  std::lock_guard<std::mutex> lock(mtx_id);
  return ++g_id_cnt;
}

uint64_t collisioncnt = 0;
uint64_t initposcnt = 0;
uint64_t unresolvedn1cnt = 0;
uint64_t fiftymovedrawcnt = 0;
int checkmate = 0;

std::mutex unresolved_mtx;

PosMap initpos;
PosMap unresolved;
PosMap resolved;

bool stop = false;

void ctrl_c_handler(int s) {
  stop = true;
}

void set_stop_handler()
{
  struct sigaction sigIntHandler;

  sigIntHandler.sa_handler = ctrl_c_handler;
  sigemptyset(&sigIntHandler.sa_mask);
  sigIntHandler.sa_flags = 0;

  sigaction(SIGINT, &sigIntHandler, NULL);

}

void insert_unresolved(PositionPacked& pp, PosInfo& pi)
{
    unresolved.insert({pp,pi});
}

void worker(int level, std::string base_path)
{
  std::cout << std::this_thread::get_id() << " starting level " << level << std::endl;

  PositionFile f_initpos(base_path, "init_pos", level);
  PositionFile f_downlevel(base_path, "init_pos", level - 1);

  time_t start = time(0);

  int loop_cnt{0};
  while (!stop) {
    PositionPacked base_pos;
    PosInfo        base_info;
    PosMap::iterator itr;
    { // dummy scope
      std::lock_guard<std::mutex> lock(unresolved_mtx);
      itr = unresolved.begin();
      if (itr != unresolved.end()) {
        base_pos  = itr->first;
        base_info = itr->second;
        unresolved.erase(base_pos);
      }
    }

    if (base_pos.pop == 0) {
      using namespace std::chrono_literals;
      std::this_thread::sleep_for(500ms);
      continue;
    }

    Board b(base_pos);

    MoveList moves;
    Side s = b.gi().getOnMove();

    b.get_all_moves(s, moves);
    int pawn_moves{0};

    if (moves.size() == 0) {
      // no moves - so either checkmate or stalemate
      bool onside_in_check = b.test_for_attack(b.getPosition().get_king_pos(s), s);
      base_info.egr = (onside_in_check) ? EGR_CHECKMATE : EGR_14A_STALEMATE;
      checkmate++;
      std::cout << std::this_thread::get_id() << " checkmate/stalemate:" << b.getPosition().fen_string() << std::endl;
    } else {
      base_info.move_cnt = moves.size();

      for (Move mv : moves) {
        Board bprime(base_pos);
        bool isPawnMove = bprime.process_move(mv, bprime.gi().getOnMove());
        // we need to flip the on-move
        Position pprime = bprime.getPosition();
        pprime.gi().toggleOnMove();
        // std::cout << std::this_thread::get_id() << ' ' << pprime.fen_string() << std::endl;
        PositionPacked posprime = pprime.pack();
        PosInfo posinfo(get_id(), base_info, mv.pack());

        // 50-move rule: drawn game if no pawn move or capture in the last 50 moves.
        // hence, if this is a pawn move or a capture, reset the counter.
        if (isPawnMove || mv.getAction() == MV_CAPTURE) {
          posinfo.fifty_cnt = 0;  // pawn move - reset 50-move counter
        }

        if (isPawnMove) {
          // new initial position
          initposcnt++;
          pawn_moves++;
          // initpos->insert({posprime,posinfo});
          f_initpos.write(posprime,posinfo);
        } else if (posinfo.fifty_cnt == 50) {
          // if no pawn move or capture in past fifty moves, draw the game
          base_info.egr = EGR_14F_50_MOVE_RULE;
          fiftymovedrawcnt++;
        } else if (bprime.gi().getPieceCnt() == level-1) {
          unresolvedn1cnt++;
          f_downlevel.write(posprime,posinfo);
        } else if(bprime.gi().getPieceCnt() == level) {
          auto itr = resolved.find(posprime);
          if (itr != resolved.end()) {
            itr->second.add_cycle(mv, posinfo.id);
            collisioncnt++;
          } else {
            itr = unresolved.find(posprime);
            if ( itr != unresolved.end() ) {
              itr->second.add_cycle(mv, posinfo.id);
              collisioncnt++;
            } else {
              unresolved.insert({posprime,posinfo});
            }
          }
        } else {
          std::cout << "ERROR! too many captures" << std::endl;
          exit(1);
        }
      }

      resolved.insert({base_pos, base_info});

      std::cout << /*std::this_thread::get_id() <<*/ " base:" << base_info.id
                << " mov/p:" << moves.size() << '/' << pawn_moves
                << " src:" << base_info.src << " mov:" << Move::unpack(base_info.move)
                << " dist:" << base_info.distance << " d50:" << base_info.fifty_cnt
                << " C:" << collisioncnt
                << " I:" << initposcnt // << initpos->size()
                << " R:" << resolved.size()
                << " U:" << unresolved.size()
                << " U1:" << unresolvedn1cnt
                << " 50:" << fiftymovedrawcnt
                << ' ' << b.getPosition().fen_string(base_info.distance)
                << std::endl;
    }
    // dump_map(initpos);
    // dump_map(unresolved);
    // dump_map(resolved);
    if (unresolved.size() == 0) {
        std::cout << std::this_thread::get_id() << " traversal finished" << std::endl;
        stop = true;
    }
  }

  std::cout << std::this_thread::get_id() << " writing resolved positions" << std::endl;
  PositionFile f_resolved(base_path, "resolved", level);
  for (auto e : resolved)
    f_resolved.write(e.first, e.second);

  time_t stop = time(0);
  double hang = std::difftime(stop, start);

  std::cout << std::this_thread::get_id() << '\n'
            << std::asctime(std::localtime(&start))
            << std::asctime(std::localtime(&stop))
            << ' ' << hang << '(' << (hang/60.0) << ") secs"
            << std::endl;

  std::cout << std::this_thread::get_id() << " stopping" << std::endl;
}