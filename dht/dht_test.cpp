#include <iostream>

#include "dht.h"

typedef uint64_t HashType;
struct Key
{
    char a[128];
};

struct Val
{
    char b[128];
};


cdfl::dht<Key, Val> aaa("/home/codefool/work/test", "bogus");


int main(int argc, char **argv)
{
    Key k;
    auto hash = aaa.hash(k);
    std::string id = aaa.get_buck_id(hash);
    std::cout << hash << ' ' << id << ' ' << aaa.get_bucket_fspec(id) << ' ' << (int)cdfl::MAX_COLL << std::endl;
    aaa.get_bucket(id);
    return 0;
}