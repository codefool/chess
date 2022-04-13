#include "db.h"
#include "md5.h"

#define FILE_BUFF_SIZE 1024*1024

std::map<size_t, std::shared_ptr<unsigned char>> BucketFile::buff_map;

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

long BucketFile::search(const unsigned char *data, size_t data_size)
{
    std::lock_guard<std::mutex> lock(_mtx);
    int max_item_cnt = FILE_BUFF_SIZE / data_size;
    std::shared_ptr<unsigned char> buff = get_file_buff();
    std::fseek(_fp, 0, SEEK_SET);
    size_t rec_cnt = std::fread(buff.get(), data_size, max_item_cnt, _fp);
    while ( rec_cnt > 0 )
    {
        unsigned char *p = buff.get();
        for ( int i(0); i < rec_cnt; ++i )
        {
            if ( !std::memcmp(p, data, data_size) )
                return (long)(p - buff.get());
            p += data_size;
        }
        rec_cnt = std::fread(buff.get(), data_size, max_item_cnt, _fp);
    }
    return -1;
}

void BucketFile::append(const unsigned char *data, size_t data_size)
{
    std::lock_guard<std::mutex> lock(_mtx);
    std::fseek(_fp, 0, SEEK_END);
    std::fwrite(data, data_size, 1, _fp);
}

// maintain a map of file buffers - one for each thread
std::shared_ptr<unsigned char> BucketFile::get_file_buff()
{
    std::hash<std::thread::id> hasher;
    size_t id_hash = hasher(std::this_thread::get_id());
    auto itr = buff_map.find(id_hash);
    if (itr == buff_map.end())
    {
        buff_map[id_hash] = std::shared_ptr<unsigned char>(new unsigned char[FILE_BUFF_SIZE]);
    }
    return buff_map[id_hash];
}

//////////////////////////////////////////////////////////////////////////////
// DiskHashTable
//
DiskHashTable::DiskHashTable(
    const std::string path_name,
    const std::string base_name,
    int               level,
    size_t            rec_len
)
: name(base_name), reclen(rec_len), reccnt(0)
{
    std::stringstream ss;
    ss << path_name << level << '/';
    path = ss.str();
    std::filesystem::create_directories(path);
}

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

long DiskHashTable::insert(const unsigned char *data)
{
    std::string bucket = calc_bucket_id(data);
    BucketFile *bp = get_bucket(bucket);
    long recno = bp->search(data, reclen);
    if (  recno == -1 )
    {
        bp->append(data, reclen);
        reccnt++;
    }
    return recno;
}

void DiskHashTable::append(const unsigned char *data)
{
    std::string bucket = calc_bucket_id(data);
    append(bucket, data);
    reccnt++;
}

void DiskHashTable::append(const std::string& bucket, const unsigned char *data)
{
    BucketFile *bp = get_bucket(bucket);
    bp->append(data, reclen);
    reccnt++;
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
