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
#include <filesystem>
#include <map>
#include <mutex>
#include <string>
#include <sstream>

#include "constants.h"
#include "md5.h"

struct BucketFile
{
    static std::map<size_t, std::shared_ptr<unsigned char>> buff_map;
    std::mutex  _mtx;
    std::FILE*  _fp;
    std::string _fspec;

    BucketFile(std::string fspec);
    ~BucketFile();
    long search(const unsigned char *data, size_t data_size);
    void append(const unsigned char *data, size_t data_size);
    size_t seek();
    std::shared_ptr<unsigned char> get_file_buff();
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
    size_t                             reccnt;

public:
    DiskHashTable(
        const std::string path_name,
        const std::string base_name,
        int               level,
        size_t            rec_len);
    virtual ~DiskHashTable();

    size_t size() const {return reccnt;}
    std::string calc_bucket_id(const unsigned char *data);
    bool search(const unsigned char *data);
    long insert(const unsigned char *data);
    void append(const unsigned char *data);
    void append(const std::string& bucket, const unsigned char *data);

    static std::string get_bucket_fspec(const std::string path, const std::string base, const std::string bucket);

private:
    BucketFile* get_bucket(const std::string& bucket);
    std::string get_bucket_fspec(const std::string& bucket);
};



