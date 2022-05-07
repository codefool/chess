#include "csvtools.h"

namespace dreid {

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
    info.parent    = get_uint64(line);
    pos.gi.i       = get_uint32(line);
    pos.pop        = get_uint64(line);
    pos.hi         = get_uint64(line);
    pos.lo         = get_uint64(line);
    info.move_cnt  = get_int(line);
    info.move.i    = get_uint16(line);
    info.distance  = get_uint16(line);
    info.egr       = static_cast<EndGameReason>(get_int(line));
}

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
        } else {
            lhs.insert({pos,info});
        }
      }
      fs.close();
      std::cout << reccnt << ' ' << dupe_cnt << std::endl; //print the data of the string
   }
   return dupe_cnt;
}

} // namespace dreid