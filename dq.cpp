#include "dq.h"

QueueBaseFile::QueueBaseFile()
: _fp(nullptr)
{}

QueueBaseFile::~QueueBaseFile()
{
    if (_fp != nullptr)
        std::fclose(_fp);
    _fp = nullptr;
}

// return true if file existed
bool QueueBaseFile::init(std::string fspec)
{
    _fspec = fspec;
    std::filesystem::path path(fspec);
    bool exists = std::filesystem::exists(path);
    const char *mode = (exists) ? "r+" : "w+";
    _fp = std::fopen( _fspec.c_str(), mode );
    return exists;
}

QueueIndex::QueueIndex()
: QueueBaseFile()
{}

bool QueueIndex::init(std::string fspec)
{
    bool exists = QueueBaseFile::init(fspec);
    if ( !exists )
    {
        // initialize the index file
        std::fwrite(&_header, sizeof(QueueHeader), 1, _fp);
    }
    else
    {
        std::fread(&_header, sizeof(QueueHeader), 1, _fp);
    }
    return exists;
}


QueueData::QueueData()
: QueueBaseFile()
{}

bool QueueData::init(std::string fspec)
{
    bool exists = QueueBaseFile::init(fspec);
    if ( !exists )
    {
        // initialize the data file
    }
    return exists;
}


DiskQueue::DiskQueue(std::string path, std::string name)
: _path(path), _name(name)
{
    // queue is in its own folder
    // path/name/name.idx and name.dat
    std::filesystem::path fpath(path);
    std::stringstream ss;
    ss << path << '/' << name;
    std::filesystem::create_directories(ss.str());
    ss << '/' << name;
    _idx.init(ss.str() + ".idx");
    _dat.init(ss.str() + ".dat");
}

DiskQueue::~DiskQueue()
{}

void DiskQueue::push()
{
    // add record to the end of the queue
    // if current block is full, allocate a new block
    // - move head of free chain to tail of alloc chain
    // - if free chain is empty, add new block at end of file
}

void DiskQueue::pop()
{
    // pop record off the top of the queue
    // if last record in the block, move block to end of
    // free chain
}
