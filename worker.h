#pragma once

#include <fstream>
#include <string>

#include "constants.h"
#include "db.h"

void set_global_id_cnt(PositionId id);
void insert_unresolved(PositionPacked& pp, PosInfo& pi);
PositionId get_position_id(int level);
void set_stop_handler();
void worker(int level, std::string base_path);
void write_resolved(int level, std::string& base_path, int max_distance = 0);
void write_pawn_init_pos(int level, std::string& base_path);
void write_results(PosMap& map, int level, std::string& base_path, std::string disp_name, bool use_thread_id = false);


