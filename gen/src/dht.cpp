#include <sys/stat.h>
#include "dht.h"
#include "md5.h"

namespace dreid {

#define TABLE_BUFF_SIZE 1024*1024*4 // 4 MiB

std::map<size_t, BuffPtr> DiskHashTable::BucketFile::buff_map;
// fopen is failing with errno 24 (too many files) on unlimited ulimit,
// so postulating that I'm opening file too fast.
std::mutex fopen_mtx;

DiskHashTable::BucketFile::BucketFile(std::string fspec, size_t key_len, size_t val_len)
: _fspec(fspec)
, _keylen(key_len)
, _vallen(val_len)
, _reclen(key_len + val_len)
, _reccnt(0)
, _fp(nullptr)
{
    std::lock_guard<std::mutex> lock(fopen_mtx);
    if ( open() )
    {
        struct stat stat_buf;
        if ( !stat( fspec.c_str(), &stat_buf ) )
            _reccnt = stat_buf.st_size / _reclen;
        close();
    }
}

DiskHashTable::BucketFile::~BucketFile()
{
    close();
}

bool DiskHashTable::BucketFile::open()
{
    if ( _fp == nullptr )
    {
        const char *mode = (std::filesystem::exists(_fspec)) ? "r+" : "w+";
        _fp = std::fopen( _fspec.c_str(), mode );
        if ( _fp == nullptr )
        {
            std::cout << "Error opening bucket file " << _fspec << ' ' << errno << " - terminating" << std::endl;
            return false;
        }
    }
    return true;
}

bool DiskHashTable::BucketFile::close()
{
    if ( _fp != nullptr )
    {
        std::fclose( _fp );
        _fp = nullptr;
    }
    return true;
}

off_t DiskHashTable::BucketFile::search(ucharptr_c key, ucharptr val)
{
    std::lock_guard<std::mutex> lock(_mtx);
    return search_nolock(key, val);
}

off_t DiskHashTable::BucketFile::search_nolock(ucharptr_c key, ucharptr val)
{
    file_guard fg(*this);
    int max_item_cnt = TABLE_BUFF_SIZE / _reclen;
    BuffPtr buff = get_file_buff();
    std::fseek(_fp, 0, SEEK_SET);
    fpos_t pos;  // file position at start of block
    std::fgetpos(_fp, &pos);
    size_t rec_cnt = std::fread(buff.get(), _reclen, max_item_cnt, _fp);
    while ( rec_cnt > 0 )
    {
        unsigned char *p = buff.get();
        for ( int i(0); i < rec_cnt; ++i )
        {
            if ( !std::memcmp( p, key, _keylen ) )
            {
                if ( _vallen != 0 && val != P_NAUGHT )
                    std::memcpy( val, p + _keylen, _vallen );
                off_t off = pos.__pos + ( p - buff.get() );
                return off;
            }
            p += _reclen;
        }
        std::fgetpos( _fp, &pos );
        rec_cnt = std::fread( buff.get(), _reclen, max_item_cnt, _fp );
    }
    return -1;
}


bool DiskHashTable::BucketFile::append( ucharptr_c key, ucharptr_c val )
{
    std::lock_guard<std::mutex> lock( _mtx );
    return append_nolock( key, val );
}

bool DiskHashTable::BucketFile::append_nolock( ucharptr_c key, ucharptr_c val )
{
    fpos_t pos;
    file_guard fg(*this);
    std::fseek( _fp, 0, SEEK_END );
    std::fgetpos( _fp, &pos );
    std::fwrite( key, _keylen, 1, _fp );
    if ( _vallen != 0 )
    {
        if ( val != P_NAUGHT )
            std::fwrite( val, _vallen, 1, _fp );
        else
            std::fwrite( P_NAUGHT, 1, _vallen, _fp );
    }
    _reccnt++;
    return true;
}

bool DiskHashTable::BucketFile::update(ucharptr_c key, ucharptr_c val)
{
    std::lock_guard<std::mutex> lock( _mtx );
    return update_nolock( key, val );
}

bool DiskHashTable::BucketFile::update_nolock(ucharptr_c key, ucharptr_c val)
{
    file_guard fg(*this);
    off_t pos = search_nolock( key );
    if( pos != -1 )
    {
        std::fseek( _fp, pos, SEEK_SET );
        std::fwrite( key, _keylen, 1, _fp );
        if ( _vallen != 0 )
        {
            if ( val != P_NAUGHT )
                std::fwrite( val, _vallen, 1, _fp );
            else
                std::fwrite( P_NAUGHT, 1, _vallen, _fp );
        }
        return true;
    }
    return false;
}

// read a specific record from the file. Return true
// if record was read, or false if EOF.
bool DiskHashTable::BucketFile::read( size_t recno, ucharptr key, ucharptr val )
{
    std::lock_guard<std::mutex> lock( _mtx );
    file_guard fg(*this);
    BuffPtr buff = get_file_buff();
    off_t pos = recno * _reclen;
    std::fseek( _fp, pos, SEEK_SET );
    if ( std::fread( buff.get(), _reclen, 1, _fp ) != EOF )
    {
        ucharptr p = buff.get();
        std::memcpy( key, p, _keylen );
        if ( _vallen != 0 )
            std::memcpy( val, p + _keylen, _vallen );
        return true;
    }
    return false;
}

// maintain a map of file buffers - one for each thread
BuffPtr DiskHashTable::BucketFile::get_file_buff()
{
    std::hash<std::thread::id> hasher;
    size_t id_hash = hasher( std::this_thread::get_id() );
    if ( buff_map.find( id_hash ) == buff_map.end() )
    {
        buff_map[ id_hash ] = BuffPtr(
            new unsigned char[ TABLE_BUFF_SIZE ],
            std::default_delete<uchar[]>()
        );
    }
    return buff_map[ id_hash ];
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
    std::filesystem::create_directories( path );
    return true;
}

DiskHashTable::~DiskHashTable()
{}

std::string DiskHashTable::calc_bucket_id( ucharptr_c key )
{
    return buckfunc( key, keylen );
}

bool DiskHashTable::search( ucharptr_c key, ucharptr val )
{
    std::string bucket = calc_bucket_id( key );
    BucketFilePtr bp = get_bucket( bucket );
    return bp != nullptr && bp->search( key, val ) != -1;
}

bool DiskHashTable::insert( ucharptr_c key, ucharptr_c val )
{
    std::string bucket = calc_bucket_id( key );
    BucketFilePtr bp = get_bucket( bucket );
    bool ok = bp != nullptr;
    if ( ok )
        ok = bp->search( key, val ) == -1;
    if ( ok )
        ok = bp->append( key, val );
    if ( ok )
        reccnt++;
    return ok;
}

bool DiskHashTable::append( ucharptr_c key, ucharptr_c val )
{
    std::string bucket = calc_bucket_id( key );
    BucketFilePtr bp = get_bucket( bucket );
    bool ok = bp != nullptr;
    if ( ok )
        ok = bp->append( key, val );
    if ( ok )
        reccnt++;
    return ok;
}

bool DiskHashTable::update( ucharptr_c key, ucharptr_c val )
{
    std::string bucket = calc_bucket_id( key );
    BucketFilePtr bp = get_bucket( bucket );
    return bp != nullptr && bp->update( key, val );
}

// return the file pointer for the given bucket
// Open the file pointer if it's not already
DiskHashTable::BucketFilePtr DiskHashTable::get_bucket( const std::string& bucket, bool must_exist )
{
    auto itr = fp_map.find( bucket );
    if ( itr != fp_map.end() )
        return itr->second;

    bool exists;
    std::string fspec = get_bucket_fspec( bucket, &exists );
    BucketFilePtr bf = nullptr;
    if ( exists || !must_exist )
    {
        bf = std::make_shared<BucketFile>( fspec, keylen, vallen );
        if ( bf != nullptr )
            fp_map.insert( {bucket, bf} );
    }
    return bf;
}

std::string DiskHashTable::get_bucket_fspec( const std::string& bucket, bool* exists )
{
    return DiskHashTable::get_bucket_fspec( path, name, bucket, exists );
}

std::string DiskHashTable::get_bucket_fspec( const std::string path, const std::string base, const std::string bucket, bool* exists )
{
    std::stringstream ss;
    ss << path << '/' << base << '_' << bucket;
    if ( exists != nullptr )
        *exists = std::filesystem::exists( ss.str() );
    return ss.str();
}

std::string DiskHashTable::default_hasher( ucharptr_c key, size_t keylen )
{
    MD5 md5;
    md5.update( key, keylen );
    md5.finalize();
    return md5.hexdigest().substr( 0, BUCKET_ID_WIDTH );
}

} // namespace dreid