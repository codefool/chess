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
        struct file_guard
        {
            BucketFile& _bf;
            bool        _was_open;
            file_guard(BucketFile& bf) : _bf(bf)
            {
                _was_open = _bf._fp != nullptr;
                if ( !_was_open )
                    _bf.open();
            }

            ~file_guard()
            {
                if ( !_was_open )
                    _bf.close();
            }
        };

        static std::map<size_t, BuffPtr> buff_map;

        std::mutex  _mtx;
        std::FILE*  _fp;
        std::string _fspec;
        size_t      _keylen;
        size_t      _vallen;
        size_t      _reccnt;
        size_t      _reclen;

        BucketFile( std::string fspec,
                    size_t key_len,
                    size_t val_len = 0);
        ~BucketFile();
        bool open();
        bool close();
        off_t search(ucharptr_c key, ucharptr   val = P_NAUGHT);
        bool  append(ucharptr_c key, ucharptr_c val = P_NAUGHT);
        bool  update(ucharptr_c key, ucharptr_c val = P_NAUGHT);
        bool  read(size_t recno, ucharptr key, ucharptr val);

        size_t seek();
        BuffPtr get_file_buff();

        off_t search_nolock(ucharptr_c key, ucharptr val = P_NAUGHT);
        bool  append_nolock(ucharptr_c key, ucharptr_c val = P_NAUGHT);
        bool  update_nolock(ucharptr_c key, ucharptr_c val = P_NAUGHT);
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
        const std::string bucket,
        bool *exists = nullptr);

private:
    std::string calc_bucket_id( ucharptr_c key );
    BucketFilePtr get_bucket( const std::string& bucket, bool must_exist = false );
    std::string get_bucket_fspec( const std::string& bucket, bool* exists = nullptr );
protected:
    static std::string default_hasher(ucharptr_c key, size_t keylen);
};


template <class K, class V>
class dht : public DiskHashTable
{
public:
    typedef std::pair<K,V> KeyVal;

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
        KeyVal,
        KeyVal,
        const KeyVal*,
        KeyVal>
    {
        friend dht;
    private:
        BucketFilePtrMap&    _map;
        BucketFilePtrMapCItr _buck;
        long                 _recno;
        bool                 _read;
        KeyVal               _keyval;
    public:
        iterator(BucketFilePtrMap& m, BucketFilePtrMapCItr pos)
        : _map(m), _buck(pos), _recno(0), _read(false)
        {}

        iterator& operator++()
        {
            if ( _recno >= _buck->second->_reccnt )
            {
                ++_buck;
                _recno = 0;
            }
            else
            {
                ++_recno;
            }
            _read = false;
            return *this;
        }

        iterator operator++(int)
        {
            iterator ret = *this;
            ++(*this);
            return ret;
        }

        iterator& operator--()
        {
            if ( _recno == 0 )
            {
                --_buck;
                _recno = _buck->second->_reccnt - 1;
            }
            else
            {
                --_recno;
            }
            _read = false;
            return *this;
        }

        iterator operator--(int)
        {
            iterator ret = *this;
            --(*this);
            return ret;
        }

        bool operator==(const iterator& other) const
        {
            return _buck == other._buck && _recno == other._recno;
        }

        bool operator!=(const iterator& other) const
        {
            return !(*this == other);
        }

        const KeyVal* operator->()
        {
            if ( !_read )
            {
                _buck->second->read( _recno, (ucharptr)&_keyval.first, (ucharptr)&_keyval.second );
                _read = true;
            }
            return &_keyval;
        }
    };

    iterator begin()
    {
        return iterator{fp_map, fp_map.begin()};
    }
    iterator end()
    {
        return iterator{fp_map, fp_map.end()};
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