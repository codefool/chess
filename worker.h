#pragma once

#include <fstream>
#include <string>

#include "constants.h"
#include "db.h"

class PositionFile {
private:
  std::string   fspec;
  std::ofstream ofs;
  int            line_cnt;
public:
  PositionFile(std::string base_path, std::string base_name, int level);
  ~PositionFile();
  void write(const PositionPacked& pos, const PosInfo& info);
};

void set_global_id_cnt(PositionId id);
void insert_unresolved(PositionPacked& pp, PosInfo& pi);
PositionId get_position_id(int level);
void set_stop_handler();
void worker(int level, std::string base_path);
void write_resolved(int level, std::string& base_path);
void write_pawn_init_pos(int level, std::string& base_path);
void write_results(PosMap& map, int level, std::string& base_path, std::string disp_name);


