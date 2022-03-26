#include <iostream>
#include <fstream>
#include <string>
#include <sstream>

#include "constants.h"

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

uint64_t dupeCnt{0};
int load_csv(std::string filename, PosMap& map) {
   std::fstream fs;
   std::cout << "Loading " << filename << '\n';
   fs.open(filename, std::ios::in); //open a file to perform read operation using file object
   uint64_t reccnt{0};
   if (fs.is_open()){   //checking whether the file is open
      std::string tp;
      std::getline(fs,tp); // throw away first line (column heads)
      while(std::getline(fs, tp)){ //read data from file object and put it into string.
        if ((++reccnt % 100) == 0)
            std::cout << reccnt << "\r"; //print the data of the string
        // split by comma
        std::string tok;
        std::istringstream line;
        PositionPacked pos;
        PosInfo info;
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

        PosMap::iterator itr = map.find(pos);
        if (itr != map.end()) {
            // duplicate entry
            dupeCnt++;
            std::cout << "\t dupe " << itr->first << ' ' << pos << std::endl;
        } else {
            map.insert({pos,info});
        }
      }
      fs.close();
      std::cout << std::endl;
   }
   return map.size();
}

PosMap lhs;
PosMap rhs;

int main() {
    load_csv("/mnt/c/tmp/cg_8/32/resolved_32_139782223079232.csv", lhs);
    load_csv("/mnt/c/tmp/cg_7/32/resolved_32_140492346263360.csv", rhs);

    std::cout << "lhs " << lhs.size() << " rhs " << rhs.size() << std::endl;

    // compare lhs to rhs - remove all lhs in rhs
    uint64_t cnt{0};
    uint64_t rkill{0};
    for (auto l : lhs) {
        auto r = rhs.find(l.first);
        if (r != rhs.end()) {
            rhs.erase(r);
            rkill++;
        }
        if ((++cnt % 100) == 0) {
            std::cout << cnt << ' ' << rkill << '\r';
        }
    }
    std::cout << "\n remaining lhs " << lhs.size() << " rhs " << rhs.size() << std::endl;
    // what's left in rhs is not in lhs - so what are they?
    for (auto l : lhs) {
        Position p(l.first);
        std::cout << "l " << p.fen_string() << std::endl;
    }
    // for (auto r : rhs) {
    //     Position p(r.first);
    //     std::cout << "r " << p.fen_string() << std::endl;
    // }
    return 0;
}