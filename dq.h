// DiskQueue
//
// A disk-based queue that can grow without bound
//
// This works like a FAT. There are two files: .idx and .dat, which
// contain the block allocation linked-list and the data file itself.
//
// The idx contains:
// - Starting recno provided (if any) when the queue was created.
// - Last recno assigned
// - The number of blocks currently being managed
// - Index of the first allocated block (BLOCK_HEAD)
// - Index of the first free block (BLOCK_FREE)
//
// The Index for each block contains:
// - disposition (either FREE or ALLOC)
// - fpos_t of the start of the block
// - fpos_t of the first unused rec in the block
//
//
//
//

#pragma once

#include <filesystem>
#include <mutex>

typedef uint32_t        dq_block_id;
typedef uint32_t        dq_rec_no;
typedef uint64_t        dq_sn_t;
typedef unsigned char * dq_data_t;
typedef const dq_data_t dq_data_c_t;

#pragma pack(1)

struct IndexRec
{
    std::fpos_t _start;
    std::fpos_t _free;
    dq_block_id _next;
};

struct Pos
{
    dq_block_id _block_id;
    dq_rec_no   _rec_no;
};

struct QueueHeader
{
    size_t      _block_size;
    size_t      _rec_len;
    size_t      _recs_per_block;
    dq_block_id _alloc_head;
    dq_block_id _alloc_tail;
    dq_block_id _free_head;
    dq_block_id _free_tail;
    dq_sn_t     _init_ser_no;
    dq_sn_t     _last_ser_no;
};

#pragma pack()

class QueueBaseFile
{
protected:
    std::string _fspec;
    FILE*       _fp;
    std::mutex  _mtx;
public:
    QueueBaseFile();
    virtual ~QueueBaseFile();
    virtual bool init(std::string fspec);
    virtual void save() = 0;
};

class QueueIndex : public QueueBaseFile
{
private:
    QueueHeader  _header;

public:
    QueueIndex();
    virtual ~QueueIndex();
    virtual bool init(std::string fspec);
    virtual void save();
};

class QueueData : public QueueBaseFile
{
public:
    QueueData();
    virtual ~QueueData();
    virtual bool init(std::string fspec);
    virtual void save();
};

class DiskQueue
{
private:
    std::string _path;
    std::string _name;
    QueueIndex  _idx;
    QueueData   _dat;

public:
    DiskQueue(std::string path, std::string name);
    virtual ~DiskQueue();
    void push();
    void pop();

private:
    void add_block();
};

