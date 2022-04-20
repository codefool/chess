//
// Verify that all records in a DiskHashTable are unique, even accross buckets.
//
#include <iostream>
#include <cassert>
#include <filesystem>
#include <fstream>
#include <set>
#include <sstream>

#include "constants.h"

void usage(std::string prog)
{
    std::cout << "DiskHashTable Utility\n"
              << "usage:\n"
              << '\t' << prog << " verify <path_to_dht_root> <dht_base_name> [options]\n"
              << '\t' << prog << " test [options]\n"
              << "note: there are no options yet\n"
              << std::endl;
    exit(1);
}

void command_verify(int argc, char **argv)
{
    if (argc < 4)
        usage(argv[0]);

    std::set<PositionPacked> cache;
    int rec_cnt(0);
    int dupe_cnt(0);

    std::filesystem::path path(argv[2]);
    if (!std::filesystem::exists(path))
    {
        std::cerr << path << " does not exist" << std::endl;
        exit(1);
    }
    std::string base(argv[3]);
    std::string min_buck;
    std::string max_buck;
    int frec_min(999999999);
    int frec_max(-1);
    for (int i = 0; i < 256; i++)
    {
        char bucket[3];
        std::sprintf(bucket, "%02x", i);

        std::string fspec = DiskHashTable::get_bucket_fspec(path, base, bucket);
        if (!std::filesystem::exists(fspec))
        {
            std::cerr << fspec << " does not exist" << std::endl;
            exit(2);
        }
        std::FILE *fp = std::fopen( fspec.c_str(), "r" );
        if ( fp == nullptr )
        {
            std::cerr << "Error opening bucket file " << fspec << ' ' << errno << " - terminating" << std::endl;
            exit(errno);
        }
        PositionPacked pp;
        PosInfo pi;
        int frec_cnt(0);
        while (std::fread(&pp, sizeof(PositionPacked), 1, fp) == 1)
        {
            std::fread(&pi, sizeof(PosInfo), 1, fp);
            frec_cnt++;
            if (cache.contains(pp))
                dupe_cnt++;
            else
                cache.insert(pp);
            if ((++rec_cnt % 1000) == 0 )
                std::cout << fspec << ':' << cache.size() << ' ' << dupe_cnt << ' ' << frec_cnt << '\r' << std::flush;
        }
        std::fclose(fp);
        std::cout << std::endl;
        if ( frec_cnt < frec_min)
        {
            frec_min = frec_cnt;
            min_buck = bucket;
        }
        if ( frec_cnt > frec_max)
        {
            frec_max = frec_cnt;
            max_buck = bucket;
        }
    }
    std::cout << '\n'
              << cache.size() << ' '
              << min_buck << ':' << frec_min << ' '
              << max_buck << ':' << frec_max << ' '
              << (frec_max - frec_min)
              << std::endl;
}

void command_test(int argc, char **argv)
{
    // create a temporary dht
    // fill it with 10^4 key/values
    // read them back and verify
    // update all even-numbered keys
    // verify the changes
    // close and delete the dht
    PositionPacked i_pp, o_pp;
    PosInfo i_pi, o_pi;
    DiskHashTable dht("/home/codefool/tmp/", "temp", 888, sizeof(PositionPacked), sizeof(PosInfo));
    std::memset(&i_pp, 0x00, sizeof(PositionPacked));
    std::memset(&i_pi, 0x00, sizeof(PosInfo));
    for (int i = 0; i < 10000; ++i)
    {
        i_pp.lo = i_pi.id = i;
        dht.append((ucharptr_c)&i_pp, (ucharptr_c)&i_pi);
        bool found = dht.search((ucharptr_c)&i_pp, (ucharptr)&o_pi);
        assert(found == true);
        assert(o_pi == i_pi);
        // update
        i_pi.distance = 2 * i;
        dht.update((ucharptr_c)&i_pp, (ucharptr_c)&i_pi);
        found = dht.search((ucharptr_c)&i_pp, (ucharptr)&o_pi);
        assert(found == true);
        assert(o_pi == i_pi);
    }
}

int main(int argc, char **argv)
{
    if (argc < 2)
        usage(argv[0]);

    std::string cmd = argv[1];
    if ( cmd == "verify" )
        command_verify(argc, argv);
    else if (cmd == "test" )
        command_test(argc, argv);
    else
    {
        std::cerr << "Unknown command '" << cmd << '\'' << std::endl;
        exit(1);
    }
   return 0;
}