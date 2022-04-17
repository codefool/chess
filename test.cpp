#include <iostream>

#include "constants.h"
#include "worker.h"
#include "csvtools.h"
#include "md5.h"

// "id",           "parent","gameinfo", "population",   "hi",            "lo",            "move_cnt","move_packed","distance","50_cnt","ref_cnt","move/parent..."
// uint64,          uin1t64, uint32,    uint64,          uint64,          uint64,          int,       uint16,       int,       int,     int,      uint16/uint64
// 0000000008119239,81191b4, 20800000,  ffffc0000000ffbe,4db9abcdeeeeeeee,4c66666666531235,14,        be51,         30,        30,      4,        bde1,811928e, 401,a59f3ba, 401,e93f96e, 1c61,e94019e

PosMap lhs;

int main()
{
    std::string base_path("/mnt/c/tmp/cg/");
    // auto dupe_cnt = csv_cmp("/mnt/c/tmp/cg_7/32/init-pos-combined_32_139843531208512.csv", lhs);
    // dupe_cnt = csv_cmp("/mnt/c/tmp/cg_8/32/init-pos-combined_32_139637762422592.csv", lhs);
    // dupe_cnt = csv_cmp("/mnt/c/tmp/cg_7/31/init_pos_31_139632422450944.csv", lhs);
    // dupe_cnt = csv_cmp("/mnt/c/tmp/cg_7/31/init_pos_31_139632430843648.csv", lhs);
    // dupe_cnt = csv_cmp("/mnt/c/tmp/cg_7/31/init_pos_31_139632439236352.csv", lhs);
    // dupe_cnt = csv_cmp("/mnt/c/tmp/cg_7/31/init_pos_31_139632447629056.csv", lhs);
    // dupe_cnt = csv_cmp("/mnt/c/tmp/cg_7/31/init_pos_31_139632456021760.csv", lhs);

    std::string filz[] = {
        "resolved_32.csv"
        // "init-pos-combined_31.csv"
        // "init_pos_31_140386760644352.csv",
        // "init_pos_31_140386769037056.csv",
        // "init_pos_31_140386777429760.csv",
        // "init_pos_31_140386989344512.csv",
        // "init_pos_31_140386997737216.csv",
        // "init_pos_31_140387006129920.csv",
        // "init_pos_31_140387014522624.csv",
        // "init_pos_31_140387022915328.csv"
    };

    for(auto fil : filz) {
        auto dupe_cnt = csv_cmp(base_path + "32/" + fil, lhs);
    }
    std::cout << "lhs " << lhs.size() << std::endl;

    DiskHashTable dummy("/mnt/d/tmp/aa", "base_pos", 32, sizeof(PositionPacked), sizeof(PositionRec));

    // write_results(lhs, 31, base_path, "init-pos-combined");
    for(auto r : lhs)
    {
        dummy.append((const unsigned char *)&r.first);
    }

    Position p;
    PositionPacked pp;
    pp.gi.i = 0x20000000;
    pp.pop  = 0x7dff00000007fffd;
    pp.hi   = 0xdb9abdeeeeeeee4c;
    pp.lo   = 0x4666666665c31235;
    p.unpack(pp);
    std::cout << p.fen_string() << std::endl;

    return 0;
}