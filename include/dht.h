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
static NAUGHT_TYPE  NAUGHT   = '\0';
static NAUGHT_TYPE *P_NAUGHT = &NAUGHT;

class DiskHashTable
{
    struct BucketFile
    {
        // RAII object to close a BucketFile when it falls out of scope
        // iif it wasn't already open when the guard was created.
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

        BucketFile( std::string fspec,
                    size_t key_len,
                    size_t val_len = 0,
                    bool must_exist = false);
        ~BucketFile();
        bool open();
        bool close();
        off_t search(ucharptr_c key, ucharptr   val = P_NAUGHT);
        bool  append(ucharptr_c key, ucharptr_c val = P_NAUGHT);
        bool  update(ucharptr_c key, ucharptr_c val = P_NAUGHT);
        bool  read(size_t recno, BuffPtr buff);

        size_t seek();
        BuffPtr get_file_buff();
    };

public:
    typedef std::shared_ptr<BucketFile>          BucketFilePtr;
    typedef std::map<std::string, BucketFilePtr> BucketFilePtrMap;
    typedef BucketFilePtrMap::const_iterator     BucketFilePtrMapCItr;

protected:
    BucketFilePtrMap   fp_map;
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

    static std::string get_bucket_fspec(
        const std::string path,
        const std::string base,
        const std::string bucket);

private:
    std::string calc_bucket_id(ucharptr_c key);
    BucketFilePtr get_bucket(const std::string& bucket, bool must_exist = false);
    std::string get_bucket_fspec(const std::string& bucket);
protected:
    static std::string default_hasher(ucharptr_c key, size_t keylen);
    bool get_next(short& bucket, long& recno, ucharptr key, ucharptr val);
    bool get_prior(short& bucket, long& recno, ucharptr key, ucharptr val);
};


template <class K, class V>
class dht : public DiskHashTable
{
// private:
//     DiskHashTable _dht;
public:
    class dht_pos
    {
    public:
        BucketFilePtrMapCItr _bucket;
        long                 _recno;
        K                    key;
        V                    val;
    public:
        dht_pos(BucketFilePtrMapCItr bucket, long pos)
        : _bucket(bucket), _recno(pos)
        {}

        bool operator==(const dht_pos& other) const
        {
            return _bucket == other._bucket && _recno == other._recno;
        }

        bool operator!=(const dht_pos& other) const
        {
            return !(*this == other);
        }

        const dht_pos* operator->()
        {
            return this;
        }
    };

    // iterator for a DiskHashTable
    //
    // For this iterator, we want to return consecutive records
    // in the dht across file boundaries (i.e., the last record
    // in one bucket is followed by the first record in the next
    // bucket and vise versa.) But, we don't want to actually
    // retrieve any record before unless the iterator is
    // dereferenced (* or ->).
    class iterator : public std::iterator<
        std::input_iterator_tag,
        dht_pos,
        dht_pos,
        const dht_pos*,
        dht_pos>
    {
    private:
        BucketFilePtrMapCItr _beg;
        BucketFilePtrMapCItr _end;
        dht_pos   _pos;

        friend dht;
    public:
        explicit iterator(BucketFilePtrMapCItr beg, BucketFilePtrMapCItr end)
        : _beg(beg), _end(end), _pos(beg,0)
        {}

        iterator operator ++()
        {
            _pos._recno++;
            if ( _pos._recno >= _pos._bucket->second->_reccnt )
            {
                ++(_pos._bucket);
                _pos._recno = 0;
            }
            return *this;
        }
        iterator  operator ++(int)
        {
            iterator ret = *this;
            ++(*this);
            return ret;
        }
        iterator operator --()
        {
            _pos._recno--;
            if ( _pos._recno < 0 && _pos._bucket != _beg )
            {
                --(_pos._bucket);
                _pos._recno = _pos._bucket->second->_reccnt - 1;
            }
            return *this;
        }
        iterator  operator --(int)
        {
            iterator ret = *this;
            --(*this);
            return ret;
        }
        bool operator==(iterator other) const
        {
            return _pos == other._pos;
        }
        bool operator!=(iterator other) const
        {
            return !(_pos == other._pos);
        }
        const dht_pos& operator *() const
        {
            return _pos;
        }
        const dht_pos* operator ->() const
        {
            return &_pos;
        }
    };

    iterator begin()
    {
        return iterator{fp_map.begin(), fp_map.end()};
    }
    iterator end()
    {
        return iterator{fp_map.end(), fp_map.end()};
    }

    bool open(
        const std::string  path_name,
        const std::string  base_name,
        int                level,
        dht_bucket_id_func bucket_func = default_hasher)
    {
        size_t vsize = (typeid(V) == typeid(NAUGHT_TYPE)) ? 0 : sizeof(V);
        return DiskHashTable::open(path_name, base_name, level, sizeof(K), vsize, bucket_func);
    }

    bool search(K& key)
    {
        return search(key, NAUGHT);
    }
    bool search(K& key, V& val)
    {
        return DiskHashTable::search((ucharptr_c)&key, (ucharptr_c)&val);
    }
    bool insert(K& key)
    {
        return insert(key, NAUGHT);
    }
    bool insert(K& key, V& val)
    {
        return DiskHashTable::insert((ucharptr_c)&key, (ucharptr_c)&val);
    }
    bool append(K& key)
    {
        return append(key, NAUGHT);
    }
    bool append(K& key, V& val)
    {
        return DiskHashTable::append((ucharptr_c)&key, (ucharptr_c)&val);
    }
    bool update(K& key)
    {
        return update(key, NAUGHT);
    }
    bool update(K& key, V& val)
    {
        return DiskHashTable::update((ucharptr_c)&key, (ucharptr_c)&val);
    }
};

} // namespace dreid