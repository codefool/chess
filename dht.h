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

#include "dreid.h"
#include "md5.h"

typedef unsigned char   uchar;
typedef uchar         * ucharptr;
typedef const ucharptr  ucharptr_c;
typedef std::shared_ptr<uchar[]> BuffPtr;

typedef std::string (*dht_bucket_id_func)(ucharptr_c, size_t);

struct BucketFile
{
    static const char *p_naught;
    static std::map<size_t, BuffPtr> buff_map;
    std::mutex  _mtx;
    std::FILE*  _fp;
    std::string _fspec;
    size_t      _keylen;
    size_t      _vallen;
    size_t      _reclen;

    BucketFile(std::string fspec, size_t key_len, size_t val_len = 0);
    ~BucketFile();
    off_t search(ucharptr_c key, ucharptr   val = nullptr);
    bool  append(ucharptr_c key, ucharptr_c val = nullptr);
    bool  update(ucharptr_c key, ucharptr_c val = nullptr);

    size_t seek();
    BuffPtr get_file_buff();
};

typedef std::shared_ptr<BucketFile> BucketFilePtr;

class DiskHashTable
{
private:
    std::map<std::string, BucketFilePtr> fp_map;
    size_t             keylen;
    size_t             vallen;
    size_t             reclen;
    std::string        path;
    std::string        name;
    size_t             reccnt;
    dht_bucket_id_func buckfunc;

public:
    DiskHashTable();

    virtual ~DiskHashTable();

    bool open(
        const std::string  path_name,
        const std::string  base_name,
        int                level,
        size_t             key_len,
        size_t             val_len = 0,
        dht_bucket_id_func bucket_func = default_hasher);

    size_t size() const {return reccnt;}
    bool search(ucharptr_c key, ucharptr val = nullptr);
    bool insert(ucharptr_c key, ucharptr_c val = nullptr);
    bool append(ucharptr_c key, ucharptr_c val = nullptr);
    bool update(ucharptr_c key, ucharptr_c val = nullptr);

    static std::string get_bucket_fspec(const std::string path, const std::string base, const std::string bucket);

private:
    std::string calc_bucket_id(ucharptr_c key);
    BucketFilePtr get_bucket(const std::string& bucket);
    std::string get_bucket_fspec(const std::string& bucket);

    static std::string default_hasher(ucharptr_c key, size_t keylen);
};



