#pragma once

#include <fstream>
#include <string>

#include "constants.h"
#include "db.h"

#pragma pack(1)
struct PosCycle {
  MovePacked move;
  PositionId trg;

  PosCycle(Move m, PositionId t)
  : move{m.pack()}, trg{t}
  {}
};
#pragma pack()

typedef std::vector<PosCycle> PosCycleMap;
typedef PosCycleMap *PosCycleMapPtr;

struct PosInfo {
  PositionId     id;        // unique id for this position
  PositionId     src;       // the parent of this position
  MovePacked     move;      // the Move that created in this position
  short          move_cnt;  // number of valid moves for this position
  short          distance;  // number of moves from the initial position
  short          fifty_cnt; // number of moves since last capture or pawn move (50-move rule [14F])
  EndGameReason  egr;       // end game reason
  PosCycleMapPtr cycles;    // cycles

  PosInfo();
  PosInfo(PositionId i, PosInfo s, MovePacked m);
  void add_cycle(Move move, PositionId trg);
};

typedef std::map<PositionPacked,PosInfo> PosMap;
typedef PosMap *PosMapPtr;

class PositionFile {
private:
  std::string   fspec;
  std::ofstream ofs;
public:
  PositionFile(std::string base_path, std::string base_name, int level);
  ~PositionFile();
  void write(const PositionPacked& pos, const PosInfo& info);
};

void insert_unresolved(PositionPacked& pp, PosInfo& pi);
PositionId get_id();
void set_stop_handler();
void worker(int level, std::string base_path);
