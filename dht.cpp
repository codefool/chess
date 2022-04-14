//
// Verify that all records in a DiskHashTable are unique, even accross buckets.
//
#include <iostream>
#include <filesystem>
#include <fstream>
#include <set>
#include <sstream>

#include "constants.h"

void usage(std::string prog)
{
    std::cout << "Verify all records in DiskHashTable are unique\n"
              << "usage: " << prog << " path_to_dht_root dht_base_name [options]\n"
              << "(there are no options yet)"
              << std::endl;
    exit(1);
}

std::set<PositionPacked> cache;
int rec_cnt(0);
int dupe_cnt(0);

int main(int argc, char **argv)
{
    if (argc < 3)
        usage(argv[0]);

    std::filesystem::path path(argv[1]);
    if (!std::filesystem::exists(path))
    {
        std::cerr << path << " does not exist" << std::endl;
        exit(1);
    }
    std::string base(argv[2]);
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
        PositionPacked p;
        int frec_cnt(0);
        while (std::fread(&p, sizeof(PositionPacked), 1, fp) == 1)
        {
            frec_cnt++;
            if (cache.contains(p))
                dupe_cnt++;
            else
                cache.insert(p);
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
              << min_buck << ':' << frec_min << ' '
              << max_buck << ':' << frec_max << ' '
              << (frec_max - frec_min)
              << std::endl;

    return 0;
}