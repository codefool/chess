#include <iostream>
#include <fstream>
#include <string>
#include <sstream>

#include "constants.h"
#include "worker.h"

// "id",           "parent","gameinfo", "population",   "hi",            "lo",            "move_cnt","move_packed","distance","50_cnt","ref_cnt","move/parent..."
// uint64,          uin1t64, uint32,    uint64,          uint64,          uint64,          int,       uint16,       int,       int,     int,      uint16/uint64
// 0000000008119239,81191b4, 20800000,  ffffc0000000ffbe,4db9abcdeeeeeeee,4c66666666531235,14,        be51,         30,        30,      4,        bde1,811928e, 401,a59f3ba, 401,e93f96e, 1c61,e94019e
enum ConvType {
    CONVERT_INT  = 0,
    CONVERT_UI16 = 1,
    CONVERT_UI32 = 2,
    CONVERT_UI64 = 3
};

bool get_token(std::istringstream& line, std::string& tok)
{
    if (std::getline(line, tok, ','))
        return true;
    return false;
}

int get_int(std::istringstream& line)
{
    std::string tok;
    if (get_token(line, tok))
    {
        return std::stoi(tok, NULL, 16);
    }
    return -1;
}

uint16_t get_uint16(std::istringstream& line)
{
    std::string tok;
    if (get_token(line, tok))
    {
        return static_cast<uint16_t>(std::stoul(tok, NULL, 16));
    }
    return -1;
}

uint32_t get_uint32(std::istringstream& line)
{
    std::string tok;
    if (get_token(line, tok))
    {
        return static_cast<uint32_t>(std::stoul(tok, NULL, 16));
    }
    return -1;
}

uint64_t get_uint64(std::istringstream& line)
{
    std::string tok;
    if (get_token(line, tok))
    {
        return std::strtoull(tok.c_str(), NULL, 16);
    }
    return -1;
}

void tokenize(std::string& tp, PositionPacked& pos, PosInfo& info)
{
    // split by comma
    std::string tok;
    std::istringstream line;
    PosRef  ref;

    line.str(tp);
    int fieldcnt(0);
    int refcnt(0);
    info.id        = get_uint64(line);
    info.src       = get_uint64(line);
    pos.gi.i       = get_uint32(line);
    pos.pop        = get_uint64(line);
    pos.hi         = get_uint64(line);
    pos.lo         = get_uint64(line);
    info.move_cnt  = get_int(line);
    info.move.i    = get_uint16(line);
    info.distance  = get_uint16(line);
    info.fifty_cnt = get_uint16(line);
    info.egr       = static_cast<EndGameReason>(get_int(line));
    refcnt         = get_int(line);
    while(refcnt--)
    {
        ref.move.i = get_uint16(line);
        ref.trg    = get_uint64(line);
        info.add_ref(Move::unpack(ref.move), ref.trg);
    }
}

uint64_t dupeCnt{0};
int load_csv(std::string filename, PosMap& map)
{
   std::fstream fs;
   std::cout << "Loading " << filename << '\n';
   fs.open(filename, std::ios::in); //open a file to perform read operation using file object
   uint64_t reccnt{0};
   if (fs.is_open())   //checking whether the file is open
   {
      std::string tp;
      std::getline(fs,tp); // throw away first line (column heads)
      while(std::getline(fs, tp)) //read data from file object and put it into string.
      {
        if ((++reccnt % 100) == 0)
            std::cout << reccnt << "\r"; //print the data of the string
        PositionPacked pos;
        PosInfo info;
        tokenize(tp, pos, info);
        map.insert({pos,info});
      }
      fs.close();
      std::cout << std::endl;
   }
   return map.size();
}

uint64_t csv_cmp(std::string filename, PosMap& lhs)
{
   std::fstream fs;
   std::cout << "Comparing " << filename << '\n';
   fs.open(filename, std::ios::in); //open a file to perform read operation using file object
   uint64_t reccnt{0};
   uint64_t dupe_cnt{0};
   if (fs.is_open())   //checking whether the file is open
   {
      std::string tp;
      std::getline(fs,tp); // throw away first line (column heads)
      while(std::getline(fs, tp)) //read data from file object and put it into string.
      {
        if ((reccnt++ % 100) == 0)
            std::cout << reccnt << ' ' << dupe_cnt << '\r'; //print the data of the string
        PositionPacked pos;
        PosInfo info;
        tokenize(tp, pos, info);
        auto itr = lhs.find(pos);
        if (itr != lhs.end()) {
            dupe_cnt++;
            itr->second.add_ref(Move::unpack(info.move), info.src);
        } else {
            lhs.insert({pos,info});
        }
      }
      fs.close();
      std::cout << reccnt << ' ' << dupe_cnt << std::endl; //print the data of the string
   }
   return dupe_cnt;
}

PosMap lhs;

int main()
{
    std::string base_path("/mnt/c/tmp/cg_8/");
    auto dupe_cnt = csv_cmp("/mnt/c/tmp/cg_7/32/init-pos-combined_32_139843531208512.csv", lhs);
    dupe_cnt = csv_cmp("/mnt/c/tmp/cg_8/32/init-pos-combined_32_139637762422592.csv", lhs);
    // dupe_cnt = csv_cmp("/mnt/c/tmp/cg_7/31/init_pos_31_139632422450944.csv", lhs);
    // dupe_cnt = csv_cmp("/mnt/c/tmp/cg_7/31/init_pos_31_139632430843648.csv", lhs);
    // dupe_cnt = csv_cmp("/mnt/c/tmp/cg_7/31/init_pos_31_139632439236352.csv", lhs);
    // dupe_cnt = csv_cmp("/mnt/c/tmp/cg_7/31/init_pos_31_139632447629056.csv", lhs);
    // dupe_cnt = csv_cmp("/mnt/c/tmp/cg_7/31/init_pos_31_139632456021760.csv", lhs);

    // std::string filz[] = {
    //     "init_pos_32_139666046908160.csv",
    //     "init_pos_32_139666055300864.csv",
    //     "init_pos_32_139666063693568.csv",
    //     "init_pos_32_139666072086272.csv",
    //     "init_pos_32_139666080478976.csv",
    //     "init_pos_32_139666088871680.csv",
    //     "init_pos_32_139666097264384.csv",
    //     "init_pos_32_139666105657088.csv"
    // };

    // for(auto fil : filz) {
    //     auto dupe_cnt = csv_cmp(base_path + "32/" + fil, lhs);
    // }
    std::cout << "lhs " << lhs.size() << std::endl;

    // write_results(lhs, 32, base_path, "init-pos-combined");

    return 0;
}