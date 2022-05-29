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

namespace dreid {

#define BUCKET_ID_WIDTH 3
const unsigned short BUCKET_LO  = 0;
const unsigned short BUCKET_HI  = 1 << (4 * BUCKET_ID_WIDTH );

typedef unsigned char   uchar;
typedef uchar         * ucharptr;
typedef const ucharptr  ucharptr_c;
typedef std::shared_ptr<uchar[]> BuffPtr;

typedef std::string (*dht_bucket_id_func)(ucharptr_c, size_t);

typedef uchar NAUGHT_TYPE;
static NAUGHT_TYPE  naught   = '\0';
static NAUGHT_TYPE *p_naught = &naught;

struct BucketFile
{
    class file_guard
    {
    public:
        explicit file_guard(BucketFile& bf) : _bf(bf)
        {
            _was_open = _bf._fp != nullptr;  // already open?
            if (!_was_open)
                _bf.open();
        }

        ~file_guard()
        {
            if (!_was_open)
                _bf.close();
        }
        file_guard(const file_guard&) = delete; // prevent copying
        file_guard& operator=(const file_guard&) = delete;  // prevent assignment
    private:
        bool        _was_open;
        BucketFile& _bf;
    };

    static std::map<size_t, BuffPtr> buff_map;
    std::mutex  _mtx;
    std::FILE*  _fp;
    std::string _fspec;
    bool        _exists;
    size_t      _keylen;
    size_t      _vallen;
    size_t      _reccnt;
    size_t      _reclen;

    BucketFile(std::string fspec, size_t key_len, size_t val_len = 0, bool must_exists = false);
    ~BucketFile();
    bool open();
    bool close();
    off_t search(ucharptr_c key, ucharptr   val = nullptr);
    bool  append(ucharptr_c key, ucharptr_c val = nullptr);
    bool  update(ucharptr_c key, ucharptr_c val = nullptr);
    bool  read(size_t recno, BuffPtr& buff);

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
protected:
    static std::string default_hasher(ucharptr_c key, size_t keylen);
};


template <class K, class V>
class dht : DiskHashTable
{
private:
    DiskHashTable _dht;
public:
    // class dht_pos
    // {
    // private:
    //     short _bucket;
    //     long  _pos;
    // public:
    //     K     key;
    //     V     val;
    // public:
    //     dht_pos(short bucket, long pos)
    //     : _bucket(bucket), _pos(pos)
    //     {}

    //     bool operator==(const dht_pos& other) const
    //     {
    //         return _bucket == other._bucket && _pos == other._pos;
    //     }

    //     bool operator!=(const dht_pos& other) const
    //     {
    //         return !(*this == other);
    //     }
    // };

    // class iterator : public std::iterator<
    //     std::input_iterator_tag,
    //     dht_pos,
    //     dht_pos,
    //     const dht_pos*,
    //     dht_pos>
    // {
    // private:
    //     DiskHashTable& _dht;
    //     dht_pos        _pos;
    //     BuffPtr        _buff;
    // public:
    //     explicit iterator(DiskHashTable& t, dht_pos p = dht_pos{BUCKET_LO,0});
    //     iterator& operator ++();
    //     iterator  operator ++(int);
    //     iterator& operator --();
    //     iterator  operator --(int);
    //     bool operator==(iterator other) const;
    //     bool operator!=(iterator other) const;
    //     reference operator *() const;
    // };

    // iterator begin();
    // iterator end();
    bool open(
        const std::string  path_name,
        const std::string  base_name,
        int                level,
        dht_bucket_id_func bucket_func = default_hasher)
    {
        return _dht.open(path_name, base_name, level, sizeof(K), sizeof(V), bucket_func);
    }

    size_t size() const {return _dht.size();}
    bool search(K& key)
    {
        return search(key, naught);
    }
    bool search(K& key, V& val)
    {
        return _dht.search((ucharptr_c)&key, (ucharptr_c)&val);
    }
    bool insert(K& key)
    {
        return insert(key, naught);
    }
    bool insert(K& key, V& val)
    {
        return _dht.insert((ucharptr_c)&key, (ucharptr_c)&val);
    }
    bool append(K& key)
    {
        return append(key, naught);
    }
    bool append(K& key, V& val)
    {
        return _dht.append((ucharptr_c)&key, (ucharptr_c)&val);
    }
    bool update(K& key)
    {
        return update(key, naught);
    }
    bool update(K& key, V& val)
    {
        return _dht.update((ucharptr_c)&key, (ucharptr_c)&val);
    }
};

} // namespace dreid