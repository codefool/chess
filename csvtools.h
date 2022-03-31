#pragma once

#include <iostream>
#include <fstream>
#include <string>
#include <cstring>
#include <sstream>
#include <stdexcept>

#include "constants.h"

int load_csv(std::string filename, PosMap& map);
uint64_t csv_cmp(std::string filename, PosMap& lhs);
