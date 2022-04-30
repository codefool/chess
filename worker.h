#pragma once

#include <fstream>
#include <string>

#include "dreid.h"
#include "dht.h"
#include "dq.h"

void load_stats_file(int level, std::string fspec);
void save_stats_file(std::string fspec);
void insert_unresolved(PositionPacked& pp, PosInfo& pi);
void set_stop_handler();
bool open_tables(int level);
void worker(int level);


