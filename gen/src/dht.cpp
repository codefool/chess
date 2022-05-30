#include <sys/stat.h>
#include "dht.h"
#include "md5.h"

namespace dreid {

#define TABLE_BUFF_SIZE 1024*1024*4 // 4 MiB

std::map<size_t, BuffPtr> DiskHashTable::BucketFile::buff_map;
// fopen is failing with errno 24 (too many files) on unlimited ulimit,
// so postulating that I'm opening file too fast.
std::mutex fopen_mtx;

DiskHashTable::BucketFile::BucketFile(std::string fspec, size_t key_len, size_t val_len, bool must_exists)
: _fspec(fspec), _keylen(key_len), _vallen(val_len), _reclen(key_len + val_len)
{
    std::lock_guard<std::mutex> lock(fopen_mtx);
    _exists = std::filesystem::exists(fspec);
    if ( _exists || !must_exists )
    {
        if ( open() )
        {
            struct stat stat_buf;
            if ( !stat( fspec.c_str(), &stat_buf ) )
                _reccnt = stat_buf.st_size / _reclen;
            close();
            _exists = true;
        }
    }
}

DiskHashTable::BucketFile::~BucketFile()
{
    std::lock_guard<std::mutex> lock( _mtx );
    close();
}

bool DiskHashTable::BucketFile::open()
{
    if ( _fp == nullptr )
    {
        const char *mode = (_exists) ? "r+" : "w+";
        _fp = std::fopen( _fspec.c_str(), mode );
        if ( _fp == nullptr )
        {
            std::cout << "Error opening bucket file " << _fspec << ' ' << errno << " - terminating" << std::endl;
            return false;
        }
        _exists = true;
    }
    return true;
}

bool DiskHashTable::BucketFile::close()
{
    if ( _fp != nullptr)
    {
        std::fclose(_fp);
        _fp = nullptr;
    }
    return true;
}

off_t DiskHashTable::BucketFile::search(ucharptr_c key, ucharptr val)
{
    std::lock_guard<std::mutex> lock(_mtx);
    int max_item_cnt = TABLE_BUFF_SIZE / _reclen;
    BuffPtr buff = get_file_buff();
    file_guard fg(*this);
    std::fseek(_fp, 0, SEEK_SET);
    fpos_t pos;  // file position at start of block
    std::fgetpos(_fp, &pos);
    size_t rec_cnt = std::fread(buff.get(), _reclen, max_item_cnt, _fp);
    while ( rec_cnt > 0 )
    {
        unsigned char *p = buff.get();
        for ( int i(0); i < rec_cnt; ++i )
        {
            if ( !std::memcmp(p, key, _keylen) )
            {
                if ( val != P_NAUGHT )
                    std::memcpy(val, p + _keylen, _vallen);
                off_t off = pos.__pos + (p - buff.get());
                return off;
            }
            p += _reclen;
        }
        std::fgetpos(_fp, &pos);
        rec_cnt = std::fread(buff.get(), _reclen, max_item_cnt, _fp);
    }
    return -1;
}

bool DiskHashTable::BucketFile::append(ucharptr_c key, ucharptr_c val)
{
    fpos_t pos;
    std::lock_guard<std::mutex> lock(_mtx);
    file_guard fg(*this);
    std::fseek(_fp, 0, SEEK_END);
    std::fgetpos(_fp, &pos);
    std::fwrite(key, _keylen, 1, _fp);
    if (val != P_NAUGHT)
        std::fwrite(val, _vallen, 1, _fp);
    else if(_vallen != 0)
        std::fwrite(P_NAUGHT, 1, _vallen, _fp);
    _reccnt++;
    return true;
}

bool DiskHashTable::BucketFile::update(ucharptr_c key, ucharptr_c val)
{
    file_guard fg(*this);
    off_t pos = search(key);
    if( pos != -1 )
    {
        std::fseek(_fp, pos, SEEK_SET);
        std::fwrite(key, _keylen, 1, _fp);
        if (val != P_NAUGHT)
            std::fwrite(val, _vallen, 1, _fp);
        else if(_vallen != 0)
            std::fwrite(P_NAUGHT, 1, _vallen, _fp);
        return true;
    }
    return false;
}

// read a specific record from the file. Return true
// if record was read, or false if EOF.
bool DiskHashTable::BucketFile::read(size_t recno, BuffPtr buff)
{
    file_guard fg(*this);
    off_t pos = recno * _reclen;
    std::fseek(_fp, pos, SEEK_SET);
    return std::fread(buff.get(), _reclen, 1, _fp) != EOF;
}

// maintain a map of file buffers - one for each thread
BuffPtr DiskHashTable::BucketFile::get_file_buff()
{
    std::hash<std::thread::id> hasher;
    size_t id_hash = hasher( std::this_thread::get_id() );
    if ( buff_map.find(id_hash) == buff_map.end() )
    {
        buff_map[id_hash] = BuffPtr(
            new unsigned char[TABLE_BUFF_SIZE],
            std::default_delete<uchar[]>()
        );
    }
    return buff_map[id_hash];
}

//////////////////////////////////////////////////////////////////////////////
// DiskHashTable
//
// Default hasher
DiskHashTable::DiskHashTable()
{}

bool DiskHashTable::open(
    const std::string  path_name,
    const std::string  base_name,
    int                level,
    size_t             key_len,
    size_t             val_len,
    dht_bucket_id_func bucket_func
)
{
    name     = base_name;
    keylen   = key_len;
    vallen   = val_len;
    reclen   = key_len + val_len;
    reccnt   = 0;
    buckfunc = bucket_func;

    std::stringstream ss;
    ss << path_name << level << '/' << name << '/';
    path = ss.str();
    std::filesystem::create_directories(path);
    return true;
}

DiskHashTable::~DiskHashTable()
{}

std::string DiskHashTable::calc_bucket_id(ucharptr_c key)
{
    return buckfunc(key, keylen);
}

bool DiskHashTable::search(ucharptr_c key, ucharptr val)
{
    std::string bucket = calc_bucket_id(key);
    BucketFilePtr bp = get_bucket(bucket);
    return bp->search(key, val) != -1;
}

bool DiskHashTable::insert(ucharptr_c key, ucharptr_c val)
{
    std::string bucket = calc_bucket_id(key);
    BucketFilePtr bp = get_bucket(bucket);
    int pos = bp->search(key, val);
    if ( pos == -1 )
    {
        bool ok = bp->append(key, val);
        if (ok)
            reccnt++;
        return ok;
    }
    return false;
}

bool DiskHashTable::append(ucharptr_c key, ucharptr_c val)
{
    std::string bucket = calc_bucket_id(key);
    BucketFilePtr bp = get_bucket(bucket);
    bool ok = bp->append(key, val);
    if (ok)
        reccnt++;
    return ok;
}

bool DiskHashTable::update(ucharptr_c key, ucharptr_c val)
{
    std::string bucket = calc_bucket_id(key);
    BucketFilePtr bp = get_bucket(bucket);
    return bp->update(key, val);
}

// return the file pointer for the given bucket
// Open the file pointer if it's not already
DiskHashTable::BucketFilePtr DiskHashTable::get_bucket(const std::string& bucket, bool must_exist)
{
    auto itr = fp_map.find(bucket);
    if (itr != fp_map.end())
        return itr->second;

    std::string fspec = get_bucket_fspec(bucket);
    BucketFilePtr bf = std::make_shared<BucketFile>(fspec, keylen, vallen, must_exist);
    fp_map.insert({bucket, bf});
    return bf;
}

std::string DiskHashTable::get_bucket_fspec(const std::string& bucket)
{
    return DiskHashTable::get_bucket_fspec(path, name, bucket);
}

std::string DiskHashTable::get_bucket_fspec(const std::string path, const std::string base, const std::string bucket)
{
    std::stringstream ss;
    ss << path << '/' << base << '_' << bucket;
    return ss.str();
}

std::string DiskHashTable::default_hasher(ucharptr_c key, size_t keylen)
{
    MD5 md5;
    md5.update(key, keylen);
    md5.finalize();
    return md5.hexdigest().substr(0,BUCKET_ID_WIDTH);
}

// get the next record out of the bucket. If at the end of the bucket, then
// open the next bucket. If out of buckets, return false.
bool DiskHashTable::get_next(short& bucket, long& recno, ucharptr key, ucharptr val)
{
    BuffPtr buff = BuffPtr(new unsigned char[reclen],std::default_delete<uchar[]>());
    char *p = (char*)(buff.get());
    BucketFilePtr bf;
    while(true)
    {
        std::sprintf( p, "%0*x", BUCKET_ID_WIDTH, bucket );
        std::string bfid( p ) ;
        bf = get_bucket( bfid, true );
        if ( bf->open() )
            break;
        if ( ++bucket >= BUCKET_HI )
            // we be out of buckets. So die
            return false;
        recno = 0;
    }
    bf->read( recno, buff );
    std::memcpy( key, p, keylen );
    if (vallen)
        std::memcpy( val, p+keylen, vallen );
    return true;
}

// get the prior record out of the bucket. If at the start of the bucket, then
// open the prior bucket. If out of buckets, return false.
bool DiskHashTable::get_prior(short& bucket, long& recno, ucharptr key, ucharptr val)
{
    BuffPtr buff = BuffPtr(new unsigned char[reclen],std::default_delete<uchar[]>());
    char *p = (char*)(buff.get());
    BucketFilePtr bf;
    while(true)
    {
        std::sprintf( p, "%0*x", BUCKET_ID_WIDTH, bucket );
        std::string bfid( p ) ;
        bf = get_bucket(bfid, true);
        if ( bf->open() )
            break;
        if ( ++bucket >= BUCKET_HI )
            // we be out of buckets. So die
            return false;
        recno = 0;
    }
    bf->read(recno, buff);
    std::memcpy( &key, p, keylen );
    if (vallen)
        std::memcpy( &val, p+keylen, vallen);
    return true;
}

} // namespace dreid