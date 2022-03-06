#include <iostream>
#include <cstring>
#include <chrono>
#include <map>
#include <memory>
#include <deque>
#include <initializer_list>
#include <algorithm>
#include <signal.h>
#include <thread>

#include <mysqlx/xdevapi.h>

#include "constants.h"
#include "db.h"

//
// cg pos id

int main(int argc, char **argv) {
    std::string url = "root@localhost:33060";
    DatabaseObject db(url);
    char *end;

    int level = (int)std::strtol(argv[1], &end, 10);
    PositionId id = std::strtoull(argv[2], &end, 10);

    std::cout << level << ' ' << id << std::endl;

    PositionRecord pr = db.get_position(level, id);
    Position pos(pr.pp);

    std::cout << pos.fen_string() << std::endl;

    return 0;
}