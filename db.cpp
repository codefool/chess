#include <sstream>

#include "db.h"
#include "md5.h"

BucketFile::BucketFile(std::string fspec)
: _fspec(fspec)
{
    std::cout << "Opening bucket file " << _fspec << std::endl;
    _fp = std::fopen( _fspec.c_str(), "w+" );
    if ( _fp == nullptr )
    {
        std::cout << "Error opening bucket file " << _fspec << ' ' << errno << " - terminating" << std::endl;
        exit(1);
    }
}

BucketFile::~BucketFile()
{
    std::lock_guard<std::mutex> lock(_mtx);
    if ( _fp != nullptr)
    {
        std::fclose(_fp);
        _fp = nullptr;
    }
}

long BucketFile::search(const unsigned char *data, size_t data_len)
{
    std::lock_guard<std::mutex> lock(_mtx);
    int buff_size = data_len * 4096;
    std::unique_ptr<unsigned char> buff( new unsigned char[buff_size] );
    std::fseek(_fp, 0, SEEK_SET);
    size_t rec_cnt = std::fread(buff.get(), data_len, 4096, _fp);
    while ( rec_cnt > 0 )
    {
        unsigned char *p = buff.get();
        for ( int i(0); i < rec_cnt; ++i )
        {
            if ( !std::memcmp(p, data, data_len) )
                return (long)(p - buff.get());
            p += data_len;
        }
        rec_cnt = std::fread(buff.get(), data_len, 4096, _fp);
    }
    return -1;
}

void BucketFile::append(const unsigned char *data, size_t data_len)
{
    std::lock_guard<std::mutex> lock(_mtx);
    std::fseek(_fp, 0, SEEK_END);
    std::fwrite(data, data_len, 1, _fp);
}

DiskHashTable::DiskHashTable(
    const std::string path_name,
    const std::string base_name,
    size_t            rec_len
)
: path(path_name), name(base_name), reclen(rec_len)
{}

DiskHashTable::~DiskHashTable()
{}

std::string DiskHashTable::calc_bucket_id(const unsigned char *data)
{
    MD5 md5;
    md5.update((const unsigned char *)data, reclen);
    md5.finalize();
    return md5.hexdigest().substr(0,2);
}

bool DiskHashTable::search(const unsigned char *data)
{
    std::string bucket = calc_bucket_id(data);
    BucketFile *bp = get_bucket(bucket);
    return bp->search(data, reclen) != -1;
}

void DiskHashTable::append(const unsigned char *data)
{
    std::string bucket = calc_bucket_id(data);
    append(bucket, data);
}

void DiskHashTable::append(const std::string& bucket, const unsigned char *data)
{
    BucketFile *bp = get_bucket(bucket);
    if ( bp->search(data, reclen) == -1 )
        bp->append(data, reclen);
}


// return the file pointer for the given bucket
// Open the file pointer if it's not already
BucketFile* DiskHashTable::get_bucket(const std::string& bucket)
{
    auto itr = fp_map.find(bucket);
    if (itr != fp_map.end())
        return itr->second;

    std::string fspec = get_bucket_fspec(bucket);
    BucketFile* bf = new BucketFile(fspec);
    fp_map.insert({bucket, bf});
    return bf;
}

std::string DiskHashTable::get_bucket_fspec(const std::string& bucket)
{
    std::stringstream ss;
    ss << path << '/' << name << '_' << bucket;
    return ss.str();
}
