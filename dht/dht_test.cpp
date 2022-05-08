#include <iostream>

#include "dht.h"

typedef uint64_t HashType;

#pragma pack(1)
struct Data
{
    struct
    {
        uint32_t gi;
        uint64_t pop;
        uint64_t hi;
        uint64_t lo;
    } k;
    char b[128];
};
#pragma pack()

// template<class T>
// struct zobrist_hasher
// {
//     static const size_t hash_size = 32;
//     typedef std::string hash_t;

//     static void hash(T data, hash_t& buff)
//     {
//         MD5 md5;
//         md5.update((cdfl::ucharptr_c)&data.k, sizeof(data.k));
//         md5.finalize();
//         buff = md5.hexdigest();
//     }
//     static bool compare( T lhs, T rhs )
//     {
//         return std::memcmp(&lhs, &rhs, sizeof(Data::k)) == 0;
//     }

// };

struct zobrist_hasher
{
    static const size_t hash_size = 32;
    typedef std::string hash_t;

    static void hash(Data data, hash_t& buff)
    {
        MD5 md5;
        md5.update((cdfl::ucharptr_c)&data.k, sizeof(data.k));
        md5.finalize();
        buff = md5.hexdigest();
    }
    static bool compare( Data lhs, Data rhs )
    {
        return std::memcmp(&lhs, &rhs, sizeof(Data::k)) == 0;
    }
};

cdfl::dht<Data, zobrist_hasher> aaa("/home/codefool/work/test", "bogus");
typedef cdfl::dht<Data, zobrist_hasher>::Key aaaKey;

int main(int argc, char **argv)
{
    Data d;
    d.k.gi  = 0x12345678;
    d.k.pop = 0x0123456789ABCDEFULL;
    d.k.hi  = 0x0123456789ABCDEFULL;
    d.k.lo  = 0x0123456789ABCDEFULL;

    Data e = d;
    e.k.lo  = 0x0123456789ABCDEEULL;

    std::cout << zobrist_hasher::compare(d,e) << std::endl;

    auto hash = aaa.hash(d);
    std::string id = aaa.get_buck_id(hash);
    std::cout << hash << ' ' << id << ' ' << aaa.get_bucket_fspec(id) << ' ' << (int)cdfl::MAX_COLL << std::endl;
    aaa.get_bucket(id);
    aaaKey k;
    aaa.search(d, k);
    return 0;
}