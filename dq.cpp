#include "dq.h"

const char * dq_naught = "\0";

QueueFile::QueueFile()
: _fp(nullptr)
{}

QueueFile::~QueueFile()
{
    if (_fp != nullptr)
        std::fclose(_fp);
    _fp = nullptr;
}

// return true if file existed
bool QueueFile::open(std::string fspec)
{
    _fspec = fspec;
    std::filesystem::path path(fspec);
    bool exists = std::filesystem::exists(path);
    const char *mode = (exists) ? "r+" : "w+";
    _fp = std::fopen( _fspec.c_str(), mode );
    return exists;
}

QueueFile::operator FILE*()
{
    return _fp;
}

std::mutex& QueueFile::mtx()
{
    return _mtx;
}

DiskQueue::DiskQueue(std::string path, std::string name, dq_rec_no_t reclen)
: _path(path), _name(name)
{
    // queue is in its own folder
    // path/name/name.idx and name.dat
    std::filesystem::path fpath(path);
    std::stringstream ss;
    ss << path << '/' << name;
    std::filesystem::create_directories(ss.str());
    ss << '/' << name;
    if ( _idx.open(ss.str() + ".idx") )
    {
        std::fread(&_header, sizeof(QueueHeader), 1, _idx );
        // if (_header._reclen != reclen) fail?
        // read index
    }
    else
    {
        _header._block_cnt      = 0;
        _header._rec_len        = reclen;
        _header._recs_per_block = 1024*1024 / reclen;
        _header._block_size     = _header._recs_per_block * reclen;
        _header._alloc_head     =
        _header._alloc_tail     =
        _header._free_head      =
        _header._free_tail      = BLOCK_NIL;
        _header._push._block_id = BLOCK_NIL;
        _header._push._rec_no   = _header._recs_per_block;
        _header._pop ._block_id = BLOCK_NIL;
        _header._pop ._rec_no   = _header._recs_per_block;

        std::fwrite(&_header, sizeof(QueueHeader), 1, _idx );
    }
    _push = _header._push;
    _pop  = _header._pop;

    _dat.open(ss.str() + ".dat");
}

DiskQueue::~DiskQueue()
{}

void DiskQueue::push(const dq_data_t data)
{
    if ( _push._rec_no == _header._recs_per_block )
    {
        // current block is full - get a fresh block
        if ( _free.empty() )
        {
            // create a new block at end of file
            off_t pos = _header._block_cnt * _header._block_size;
            _push._block_id = _header._block_cnt;
            _push._rec_no   = 0;
            _alloc.push_back(_header._block_cnt);
            _header._block_cnt++;
            std::lock_guard<std::mutex> lock(_dat.mtx());
            std::fseek(_dat, pos, SEEK_SET);
            std::fwrite(dq_naught, 1, _header._block_size, _dat);
        }
        else
        {
            // pull the head off of the free chain
            dq_block_id_t block = _free.front();
            _free.pop_front();
            _alloc.push_back(block);
            _push._block_id = block;
            _push._rec_no   = 0;
        }
    }
    off_t pos = (_push._block_id * _header._block_size) +
                (_push._rec_no   * _header._rec_len);
    std::lock_guard<std::mutex> lock(_dat.mtx());
    std::fseek(_dat, pos, SEEK_SET);
    std::fwrite((const void *)data, 1, _header._rec_len, _dat);
    _push._rec_no++;
}

bool DiskQueue::pop(dq_data_t data)
{
    // pop record off the top of the queue
    // if last record in the block, move block to end of
    // free chain
    if ( _pop._rec_no == _header._recs_per_block )
    {
        // put this block on the free chain,
        // setup next block (if any)
        if ( _pop._block_id != BLOCK_NIL )
        {
            _alloc.pop_front();
            _free.push_back(_pop._block_id);
        }
        if ( _alloc.empty() )
        {
            _pop._block_id = BLOCK_NIL;
            _pop._rec_no   = _header._recs_per_block;
        }
        else
        {
            _pop._block_id = _alloc.front();
            _pop._rec_no   = 0;
        }
    }
    if ( empty() )
        return false;

    off_t pos = (_pop._block_id * _header._block_size) +
                (_pop._rec_no   * _header._rec_len);
    std::lock_guard<std::mutex> lock(_dat.mtx());
    std::fseek(_dat, pos, SEEK_SET);
    std::fread((void *)data, 1, _header._rec_len, _dat);
    _pop._rec_no++;

    return true;
}
