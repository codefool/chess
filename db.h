// db - simple database for chessgen
//
// This is not a general-purpose tool, but a bare-bones and (hopefully)
// freakishly fast way to collect positions and check for collissions
// without thrashing the disk.
//
// Use the high two-digits of MD5 as hash into 256 buckets.
// Files are binary with fixed-length records.
//
//
#pragma once
#include <cstdio>
#include <cstring>
#include <map>
#include <mutex>
#include <string>
#include <sstream>

#include "constants.h"
#include "md5.h"

struct BucketFile
{
    std::mutex  _mtx;
    std::FILE*  _fp;
    std::string _fspec;

    BucketFile(std::string fspec);
    ~BucketFile();
    long search(const unsigned char *data, size_t data_len);
    void append(const unsigned char *data, size_t data_len);
    size_t seek();
};

class DiskHashTable
{
private:
    std::map<std::string, BucketFile*> fp_map;
    size_t                             reclen;
    // the target filename for a bucket is
    // <path>/<name>_dd
    std::string                        path;
    std::string                        name;

public:
    DiskHashTable(
        const std::string path_name,
        const std::string base_name,
        size_t            rec_len);
    virtual ~DiskHashTable();

    std::string calc_bucket_id(const unsigned char *data);
    bool search(const unsigned char *data);
    void append(const unsigned char *data);
    void append(const std::string& bucket, const unsigned char *data);

private:
    BucketFile* get_bucket(const std::string& bucket);
    std::string get_bucket_fspec(const std::string& bucket);
};



