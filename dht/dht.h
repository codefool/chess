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

// cdfl::dht<PositionRec, PositionRecHasher<PositionRec>> tbl;
// tbl.search(pr, tbl::Key key); - how to get the hash back?
template<class T>
struct md5
{
    static const size_t hash_size = 32;
    typedef std::string hash_t;

    static void hash(T& data, hash_t& buff)
    {
        MD5 md5;
        md5.update((ucharptr_c)&data, sizeof(T));
        md5.finalize();
        buff = md5.hexdigest();
    }

    static bool compare( T lhs, T rhs )
    {
        return std::memcmp(&lhs, &rhs) == 0;
    }
};

struct BucketFile
{
    std::mutex  _m;
    std::FILE  *_fp;
    std::string _fspec;

    BucketFile()
    : _fp(nullptr)
    {}

    BucketFile(std::string fspec, std::FILE *fp = nullptr)
    : _fp(fp), _fspec(fspec)
    {}
};

typedef std::shared_ptr<BucketFile> FilePtr;

template<class _Data, class _Hasher = md5<_Data>>
class dht
{
public:
    typedef _Data   data_t;
    typedef _Hasher hasher_t;

    typedef typename hasher_t::hash_t hash_t;

#pragma pack(1)
    struct Key
    {
        hash_t     _h;
        coll_cnt_t _c;

        Key() {}
        Key( data_t& d )
        : _c( UNKNOWN )
        {
            hasher_t::hash(d, _h);
        }
        bool is_unknown()
        {
            return _c == UNKNOWN;
        }
    };

    struct Rec
    {
        Key     _k;
        data_t  _d;
    };
#pragma pack()

public:
    const size_t hash_len     = hasher_t::hash_size;
    const size_t data_len     = sizeof( data_t );
    const size_t key_len      = sizeof( Key );
    const size_t rec_len      = sizeof( Rec );
    const size_t max_item_cnt = TABLE_BUFF_SIZE / rec_len;

    std::string                 base_path;
    std::string                 base_name;

    std::mutex                     bucket_mtx;
    std::mutex                     coll_mtx;
    std::map<size_t, BuffPtr>      buff_map;
    std::map<std::string, FilePtr> buck_map;
    std::map<hash_t, FilePtr>      coll_map;
    short                          opencnt;

    dht(std::string fn, std::string bn)
    : base_name(bn), opencnt(0)
    {
        std::stringstream ss;
        ss << fn << '/' << bn << '/';
        base_path = ss.str();
        std::filesystem::create_directories( base_path );
    }

    ~dht()
    {
        for ( auto i : buck_map )
            if (i.second->_fp != nullptr)
            {
                fclose(i.second->_fp);
            }
    }

    hash_t hash( data_t d )
    {
        hash_t hash;
        hasher_t::hash(d, hash);
        return hash;
    }

    Key make_key(data_t d)
    {
        return Key(d);
    }

    std::string get_buck_id(uint64_t hash)
    {
        char buff[hash_len+1];
        std::sprintf(buff, "%0*lx", (int)hash_len, hash );
        return get_buck_id(std::string(buff));
    }

    std::string get_buck_id(std::string hash)
    {
        return hash.substr(0,2);
    }

    FilePtr get_bucket(std::string bucket_id)
    {
        std::lock_guard<std::mutex> lock( bucket_mtx );
        auto itr = buck_map.find(bucket_id);
        if (itr != buck_map.end())
            return itr->second;

        std::string fspec = get_bucket_fspec(bucket_id);
        std::cout << opencnt << " Opening bucket file " << fspec << std::endl;
        const char *mode = std::filesystem::exists(fspec) ? "r+" : "w+";
        FILE *fp = std::fopen( fspec.c_str(), mode );
        if ( fp == nullptr )
        {
            std::cout << "Error opening bucket file " << fspec << ' ' << errno << " - terminating" << std::endl;
            return nullptr;
        }
        opencnt++;
        FilePtr bf = std::make_shared<BucketFile>(fspec, fp);
        buck_map[ bucket_id ] = bf;
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

    BuffPtr get_file_buff()
    {
        size_t id_hash = get_thread_hash();
        if ( buff_map.find(id_hash) == buff_map.end() )
        {
            buff_map[id_hash] = BuffPtr(
                new unsigned char[TABLE_BUFF_SIZE],
                std::default_delete<uchar[]>()
            );
        }
        return buff_map[id_hash];
    }

    off_t search(data_t& data)
    {
        Key k(data);
        return search(k, data);
    }

    off_t search(Key& key, data_t& data)
    {
        std::string buck_id = get_buck_id( key._h );
        FilePtr bf = get_bucket( buck_id );
        BuffPtr buff = get_file_buff();
        {
            std::unique_lock<std::mutex> lock(coll_mtx);
            auto itr = coll_map.find(key._h);
            if ( itr != coll_map.end() )
            {
                // this hash has one or more collissions
                lock.unlock();
                return process_collission(data);
            }
        }

        std::lock_guard<std::mutex> lock( bf->_m );
        std::fseek( bf->_fp, 0, SEEK_SET );
        fpos_t pos;  // file position at start of block
        std::fgetpos( bf->_fp, &pos );
        // if the key we're looking for has no collissions (yet), its
        // _c member will be UNKNOWN. If this is the case, check just
        // the hash, otherwise key plus the collision distinguisher.
        int cmp_len =  key.is_unknown() ? hash_len : key_len;
        size_t rec_cnt = std::fread( buff.get(), rec_len, max_item_cnt, bf->_fp );
        while ( rec_cnt > 0 )
        {
            unsigned char *p = buff.get();
            for ( int i(0); i < rec_cnt; ++i )
            {
                if ( !std::memcmp( p, &key, cmp_len ) )
                {
                    // we have a hash match (at least.) If key collision is unknown,
                    // then we need to make sure that the data actually the same.
                    if ( key.is_unknown() && !hasher_t::compare( data, *(data_t*)p ) )
                    {
                        // hash collission. We have two different sets of data
                        // that return the same hash. Super rare, but super annoying.

                    }
                    off_t off = pos.__pos + ( p - buff.get() );
                    return off;
                }
                p += rec_len;
            }
            std::fgetpos( bf->_fp, &pos );
            rec_cnt = std::fread( buff.get(), rec_len, max_item_cnt, bf->_fp );
        }
        return -1;
    }

    off_t process_collission(data_t& data)
    {
        // open/create the collission file for this hash
        // then, search for the data record that matches data
        Key key(data);
        FilePtr cf;
        { // dummy scope
            std::lock_guard<std::mutex> lock( coll_mtx );
            auto itr = coll_map.find( key._h );
            if ( itr == coll_map.end() )
            {
                std::stringstream ss;
                ss << base_path << '/' << key._h << ".col";
                cf = std::make_shared<BucketFile>(ss.str());
                coll_map[ key._h ] = cf;
            }
            else
            {
                cf = itr->second;
            }
        }   // end dummy scope

        std::lock_guard<std::mutex> lock(cf->_m);
        std::cout << opencnt << " Opening collission file " << cf->_fspec << std::endl;
        const char *mode = std::filesystem::exists(cf->_fspec) ? "r+" : "w+";
        cf->_fp = std::fopen( cf->_fspec.c_str(), mode );
        if ( cf->_fp == nullptr )
        {
            std::cout << "Error opening bucket file " << cf->_fspec << ' ' << errno << " - terminating" << std::endl;
            return -1;
        }

        // scan the file as usual, looking for a data match.

        std::fclose(cf->_fp);
        return 0;
    }
};


} // namespace cdfl