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
egr{EGR_NONE}, refs{nullptr}
{}

PosInfo::PosInfo(PositionId i, PosInfo s, MovePacked m)
: id{i}, src{s.id}, move(m), move_cnt{0},
    distance{s.distance + 1},
    fifty_cnt{s.fifty_cnt + 1},
    egr{EGR_NONE}, refs{nullptr}
{}

// it's possible that multiple threads for the same position
// can cause issues here. Since we have at most THREAD_COUNT
// possible collisions, we only need that many mutex's.
// We do a poor man's hash and just id mod THREAD_COUNT
// should provide adequate protection without pausing all
// threads every time we add a reference (of which there are
// many)
std::vector<std::mutex> posrefmtx(THREAD_COUNT);

void PosInfo::add_ref(Move move, PositionId trg)
{
    std::lock_guard<std::mutex> lock(posrefmtx[id % THREAD_COUNT]);
    if (refs == nullptr) {
        refs = new PosRefMap();
        refs->reserve(10);
    }
    refs->push_back(PosRef(move,trg));
}

PositionFile::PositionFile(std::string base_path, std::string base_name, int level)
: line_cnt{0}
{
    std::stringstream ss;
    ss << base_path << level << '/';
    std::filesystem::create_directories(ss.str());
    ss << base_name << '_' << level << '_' << std::this_thread::get_id() << ".csv";
    fspec = ss.str();
    ofs.open(fspec, std::ios_base::app);
    ofs.flags(std::ios::hex);
    ofs.fill('0');
    ofs << "\"id\","
        << "\"parent\","
        << "\"gameinfo\","
        << "\"population\","
        << "\"hi\","
        << "\"lo\","
        << "\"move_cnt\","
        << "\"move_packed\","
        << "\"distance\","
        << "\"50_cnt\","
        << "\"end_game\","
        << "\"ref_cnt\","
        << "\"move/parent...\""
        << '\n';
}

PositionFile::~PositionFile()
{
    ofs << std::flush;
    ofs.close();
}

void PositionFile::write(const PositionPacked& pos, const PosInfo& info)
{
    //id    gi   pop    hi     lo     src    mv   dist 50m
    auto ow = ofs.width(16);
    ofs << info.id << ','
        << info.src << ',';
    ofs.width(8);
    ofs << pos.gi.i << ',';
    ofs.width(16);
    ofs << pos.pop << ','
        << pos.hi << ','
        << pos.lo << ',';
    ofs.width(ow);
    ofs << info.move_cnt << ','
        << info.move.i << ','
        << info.distance << ','
        << info.fifty_cnt << ','
        << static_cast<int>(info.egr) << ',';

    if (info.refs == nullptr) {
        ofs << 0;
    } else {
        ofs << info.refs->size() << ',';
        bool second = false;
        for (auto e : *info.refs) {
            if (second)
              ofs << ',';
            ofs << e.move.i << ',' << e.trg;
            second = true;
        }
    }
    ofs << '\n';
    if ((++line_cnt % 100) == 0)
      ofs << std::flush;
}

std::mutex mtx_id;
PositionId g_id_cnt = 0;

void set_global_id_cnt(PositionId id)
{
  g_id_cnt = id;
}

PositionId get_position_id()
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
std::mutex resolved_mtx;

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
    std::lock_guard<std::mutex> lock(unresolved_mtx);
    unresolved.insert({pp,pi});
}

void worker(int level, std::string base_path)
{
  std::cout << std::this_thread::get_id() << " starting level " << level << std::endl;

  PositionFile f_initpos(base_path, "init_pos", level);
  PositionFile f_downlevel(base_path, "init_pos", level - 1);
  std::stringstream ss;

  MoveList moves;
  moves.reserve(50);

  // time_t start = time(0);

  int loop_cnt{0};
  bool done = false;
  while (!stop && !done) {
    PositionPacked base_pos;
    PosInfo        base_info;
    PosMap::iterator itr;
    { // dummy scope
      std::lock_guard<std::mutex> lock0(unresolved_mtx);
      itr = unresolved.begin();
      if (itr != unresolved.end()) {
        base_pos  = itr->first;
        base_info = itr->second;
        unresolved.erase(base_pos);
        std::lock_guard<std::mutex> lock1(resolved_mtx);
        resolved.insert({base_pos,base_info});
      }
    }

    if (base_pos.pop == 0) {
      using namespace std::chrono_literals;
      std::this_thread::sleep_for(500ms);
      continue;
    }

    Board b(base_pos);
    Side  s = b.gi().getOnMove();

    moves.clear();
    b.get_all_moves(s, moves);
    int pawn_moves{0};
    int coll_cnt{0};
    int fifty_cnt{0};
    int nsub1_cnt{0};

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
        // PosInfo posinfo(get_position_id(), base_info, mv.pack());
        PosInfo posinfo(0, base_info, mv.pack());

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
          posinfo.id = get_position_id();
          f_initpos.write(posprime,posinfo);

        } else if (posinfo.fifty_cnt == 50) {
          // if no pawn move or capture in past fifty moves, draw the game
          base_info.egr = EGR_14F_50_MOVE_RULE;
          fiftymovedrawcnt++;
          fifty_cnt++;

        } else if (bprime.gi().getPieceCnt() == level-1) {
          unresolvedn1cnt++;
          posinfo.id = get_position_id();
          f_downlevel.write(posprime,posinfo);
          nsub1_cnt++;

        } else if(bprime.gi().getPieceCnt() == level) {
          bool found = false;
          { // dummy scope
            std::lock_guard<std::mutex> lock(resolved_mtx);
            auto itr = resolved.find(posprime);
            if (itr != resolved.end()) {
              itr->second.add_ref(mv, base_info.id);
              collisioncnt++;
              coll_cnt++;
              found = true;
            }
          } // end dummy scope
          if (!found)
          {
            std::lock_guard<std::mutex> lock(unresolved_mtx);
            itr = unresolved.find(posprime);
            if ( itr != unresolved.end() ) {
              itr->second.add_ref(mv, base_info.id);
              collisioncnt++;
              coll_cnt++;
            } else {
              posinfo.id = get_position_id();
              unresolved.insert({posprime, posinfo});
            }
          }
        } else {
          std::cout << "ERROR! too many captures" << std::endl;
          exit(1);
        }
      }

      { // dummy scope
        std::lock_guard<std::mutex> lock(resolved_mtx);
        resolved[base_pos] = base_info;
      } // end dummy scope

      // std::cout << "base mov/p/c/5/1 parent mov dist dis50 coll_cnt init_cnt res_cnt unr unr1 fifty FEN\n";
      ss.str(std::string());
      ss
      // std::cout /*<< std::this_thread::get_id() <<*/
                << base_info.id
                << ',' << moves.size()
                    << '/' << pawn_moves
                    << '/' << coll_cnt
                    << '/' << fifty_cnt
                    << '/' << nsub1_cnt
                << ',' << base_info.src
                << ',' << Move::unpack(base_info.move)
                << ',' << base_info.distance
                << ',' << base_info.fifty_cnt
                << ',' << collisioncnt
                << ',' << initposcnt // << initpos->size()
                << ',' << resolved.size()
                << ',' << unresolved.size()
                << ',' << unresolvedn1cnt
                << ',' << fiftymovedrawcnt
                << ',' << b.getPosition().fen_string(base_info.distance)
                << std::endl;
      std::cout << ss.str();
    }
    // dump_map(initpos);
    // dump_map(unresolved);
    // dump_map(resolved);
    if (unresolved.size() == 0) {
        std::cout << std::this_thread::get_id() << " traversal finished" << std::endl;
        done = true;
    }
  }

  // // if stop, then ctrl-c was used, so bail
  // if (!stop) {
  //   std::cout << std::this_thread::get_id() << " writing resolved positions" << std::endl;
  //   PositionFile f_resolved(base_path, "resolved", level);
  //   for (auto e : resolved)
  //     f_resolved.write(e.first, e.second);

  //   // time_t stop = time(0);
  //   // double hang = std::difftime(stop, start);

  //   // std::cout << std::this_thread::get_id() << '\n'
  //   //           << std::asctime(std::localtime(&start))
  //   //           << std::asctime(std::localtime(&stop))
  //   //           << ' ' << hang << '(' << (hang/60.0) << ") secs"
  //   //           << std::endl;
  // }
  std::cout << std::this_thread::get_id() << " stopping" << std::endl;
}

void write_resolved(int level, std::string& base_path)
{
    std::cout << std::this_thread::get_id() << " writing resolved positions" << std::endl;
    PositionFile f_resolved(base_path, "resolved", level);
    for (auto e : resolved)
      f_resolved.write(e.first, e.second);
}