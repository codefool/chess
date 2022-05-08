#pragma once

#include <cstddef>
#include <filesystem>
#include <map>
#include <memory>
#include <mutex>

#include "md5.h"

namespace cdfl {

// collission counter
typedef unsigned char  uchar;
typedef uchar         *ucharptr;
typedef const ucharptr ucharptr_c;
typedef std::shared_ptr<uchar[]> BuffPtr;


typedef uchar      coll_cnt_t;
const coll_cnt_t   MAX_COLL(-1);
const coll_cnt_t   UNKNOWN(MAX_COLL);
extern const char *p_naught;

#define TABLE_BUFF_SIZE 1024*1024   // 1MiB

size_t get_thread_hash();

// cdfl::dht<PositionHash, PositionPacked, PositionRec> tbl;
template<class T>
struct md5
{
    static const size_t hash_size = 32;
    typedef std::string hash_t;

    static void hash(T key, hash_t& buff)
    {
        MD5 md5;
        md5.update((ucharptr_c)&key, sizeof(T));
        md5.finalize();
        buff = md5.hexdigest();
    }
};

struct BucketFile
{
    std::mutex  _m;
    std::FILE  *_fp;
    std::string _fspec;

    BucketFile() {}

    BucketFile(std::FILE *fp, std::string fspec)
    : _fp(fp), _fspec(fspec)
    {}
};

typedef std::shared_ptr<BucketFile> BucketFilePtr;

template<class _Key, class _Val, class _Hasher = md5<_Key>>
class dht
{
public:
    typedef _Key    key_t;
    typedef _Val    val_t;
    typedef _Hasher hasher_t;

    typedef typename hasher_t::hash_t hash_t;

private:
#pragma pack(1)
    struct _k
    {
        hash_t     _h;
        coll_cnt_t _c;

        _k(key_t k)
        : _c(UNKNOWN)
        {
            hasher_t::hash(k, _h);
        }
    };

    struct _r
    {
        key_t   _k;
        val_t   _v;
    };
#pragma pack()

public:
    const size_t key_disk_len = sizeof( _k );
    const size_t rec_len      = sizeof( _r );
    const size_t hash_len     = hasher_t::hash_size;
    const size_t key_len      = sizeof( key_t );
    const size_t val_len      = sizeof( val_t );

    std::string                 base_path;
    std::string                 base_name;

    std::mutex                           bucket_mtx;
    std::map<size_t, BuffPtr>            buff_map;
    std::map<std::string, BucketFilePtr> buck_map;
    short                                opencnt;

    dht(std::string fn, std::string bn)
    : base_name(bn), opencnt(0)
    {
        std::stringstream ss;
        ss << fn << '/' << bn << '/';
        base_path = ss.str();
        std::filesystem::create_directories( base_path );
    }

    hash_t hash( key_t k )
    {
        _k key( k );
        return key._h;
    }

    std::string get_buck_id(uint64_t hash)
    {
        uchar buff[hash_len+1];
        std::sprintf(buff, "%0*lx", hash_len, hash );
        return get_buck_id(std::string(buff));
    }

    std::string get_buck_id(std::string hash)
    {
        return hash.substr(0,2);
    }

    BucketFilePtr get_bucket(std::string bucket_id)
    {
        std::lock_guard<std::mutex> lock( bucket_mtx );
        auto itr = buck_map.find(bucket_id);
        if (itr != buck_map.end())
            return itr->second;

        BucketFilePtr bf = std::make_shared<BucketFile>();
        std::string fspec = get_bucket_fspec(bucket_id);
        std::cout << opencnt << " Opening bucket file " << fspec << std::endl;
        const char *mode = std::filesystem::exists(bf->_fspec) ? "r+" : "w+";
        FILE *fp = std::fopen( fspec.c_str(), mode );
        if ( fp == nullptr )
        {
            std::cout << "Error opening bucket file " << fspec << ' ' << errno << " - terminating" << std::endl;
            exit(1);
        }
        opencnt++;
        buck_map[ bucket_id ] = std::make_shared<BucketFile>(fp, fspec);
        return bf;
    }

    std::string get_bucket_fspec(const std::string bucket_id)
    {
        return get_bucket_fspec(base_path, base_name, bucket_id);
    }

    std::string get_bucket_fspec(const std::string path, const std::string name, const std::string bucket_id ) const
    {
        std::stringstream ss;
        ss << path << '/' << name << '_' << bucket_id;
        return ss.str();
    }
};




} // namespace cdfl