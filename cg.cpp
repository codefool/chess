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

#include "dreid.h"
#include "dht.h"

//
// cg pos id
bool stop = false;

void ctrl_c_handler(int s) {
  stop = true;
}

int main(int argc, char **argv) {
    std::string url = "root@localhost:33060";
    DatabaseObject db(url);
    char *end;
    std::stringstream ss;

    struct sigaction sigIntHandler;

    sigIntHandler.sa_handler = ctrl_c_handler;
    sigemptyset(&sigIntHandler.sa_mask);
    sigIntHandler.sa_flags = 0;
    sigaction(SIGINT, &sigIntHandler, NULL);

    std::string cmd(argv[1]);

    if (cmd == "get") {
        int level = (int)std::strtol(argv[2], &end, 10);
        PositionId id = std::strtoull(argv[3], &end, 10);

        std::cout << level << ' ' << id << std::endl;

        PositionRecord pr = db.get_position(level, id);
        Position pos(pr.pp);

        std::cout << pos.fen_string() << std::endl;
    } else if (cmd == "poll") {
        int level = (int)std::strtol(argv[2], &end, 10);
        int last_row_cnt{0};
        int last_res_cnt{0};

        ss << "SELECT COUNT(1), max(move_cnt), max(ref_cnt) from position_" << level << ';';
        std::string sql1 = ss.str();
        ss.str(std::string());
        ss << "SELECT COUNT(1) from position_" << level << " WHERE resolved=1;";
        std::string sql2 = ss.str();
        ss.str(std::string());
        ss << "SELECT COUNT(1) from position_" << level << " WHERE endgame != 0;";
        std::string sql3 = ss.str();

        while( !stop )
        {
            mysqlx::RowResult res = db.exec(sql1);
            mysqlx::Row row = res.fetchOne();
            int row_cnt = row.get(0).get<int>();
            int mov_cnt = row.get(1).get<int>();
            int ref_cnt = row.get(2).get<int>();
            res = db.exec(sql2);
            int resolved_cnt = res.fetchOne().get(0).get<int>();
            res = db.exec(sql3);
            int endgame_cnt = res.fetchOne().get(0).get<int>();

            std::cout << "positions:" << row_cnt
                      << " (" << (row_cnt - last_row_cnt) << ')'
                      << " resolved:" << resolved_cnt
                      << " (" << (resolved_cnt - last_res_cnt) << ')'
                      << " endgame:"  << endgame_cnt
                      << " max_moves:" << mov_cnt
                      << " max_ref:" << ref_cnt
                      << std::endl;

            last_row_cnt = row_cnt;
            last_res_cnt = resolved_cnt;
            using namespace std::chrono_literals;
            std::this_thread::sleep_for(1min);
        }
    }
    return 0;
}