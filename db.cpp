#include <iostream>
#include <mutex>

#include "constants.h"
#include "db.h"

std::istream& operator>>(std::istream& is, PositionPacked& p) {
    return is;
}

std::mutex ins_mtx;

DatabaseObject::DatabaseObject(std::string& url)
: sess(url)
{
    sess.sql("USE chess;").execute();
}

void DatabaseObject::create_position_table(int level)
{
    std::stringstream ss;
    ss << "CREATE TABLE IF NOT EXISTS "
        "position_" << level << "("
            "id BIGINT unsigned NOT NULL AUTO_INCREMENT,"
            "src BIGINT unsigned NOT NULL,"
            "move SMALLINT unsigned NOT NULL,"
            "gi INT unsigned DEFAULT NULL,"
            "pop BIGINT unsigned DEFAULT NULL,"
            "hi BIGINT unsigned DEFAULT NULL,"
            "lo BIGINT unsigned DEFAULT NULL,"
            "move_cnt SMALLINT DEFAULT 0,"
            "ref_cnt SMALLINT DEFAULT 0,"
            "resolved BOOLEAN DEFAULT FALSE,"
            "endgame TINYINT DEFAULT 0,"
        "PRIMARY KEY (id),"
        "KEY src_idx (src) USING BTREE,"
        "UNIQUE KEY pos_idx (gi,pop,hi,lo) USING BTREE);";
    sess.sql(ss.str()).execute();
}

PositionRecord DatabaseObject::get_next_unresolved_position(int level)
{
    PositionRecord pr;
    std::stringstream ss;
    sess.sql("START TRANSACTION").execute();
    ss << "SELECT id, src, move, gi, pop, hi, lo FROM position_" << level
       << " WHERE resolved=false "
       << "ORDER BY id ASC "
       << "LIMIT 1 FOR UPDATE SKIP LOCKED;";
    mysqlx::RowResult res = sess.sql(ss.str()).execute();
    if (res.count() > 0) {
        mysqlx::Row row = res.fetchOne();
        pr.id     = row.get(0).get<uint64_t>();
        pr.src    = row.get(1).get<uint64_t>();
        pr.move.i = row.get(2).get<int>();
        pr.pp.gi  = row.get(3).get<uint32_t>();
        pr.pp.pop = row.get(4).get<uint64_t>();
        pr.pp.hi  = row.get(5).get<uint64_t>();
        pr.pp.lo  = row.get(6).get<uint64_t>();
        ss.str(std::string());
        ss << "UPDATE position_" << level << " SET resolved = true WHERE id=" << pr.id << ";";
        sess.sql(ss.str()).execute();
        sess.sql("COMMIT;").execute();
    } else {
        sess.sql("ROLLBACK;").execute();
        pr.id = 0;
    }
    return pr;
}

PositionRecord DatabaseObject::get_position(int level, PositionId id)
{
    PositionRecord pr;
    std::stringstream ss;
    ss << "SELECT src, move, gi, pop, hi, lo FROM position_" << level << " WHERE id=" << id << ';';
    try
    {
        mysqlx::RowResult res = sess.sql(ss.str()).execute();
        if (res.count() > 0) {
            mysqlx::Row row = res.fetchOne();
            pr.id     = id;
            pr.src    = row.get(0).get<uint64_t>();
            pr.move.i = row.get(1).get<int>();
            pr.pp.gi  = row.get(2).get<uint32_t>();
            pr.pp.pop = row.get(3).get<uint64_t>();
            pr.pp.hi  = row.get(4).get<uint64_t>();
            pr.pp.lo  = row.get(5).get<uint64_t>();
        }
    } catch(mysqlx::abi2::r0::Error e) {
        std::cerr << ss.str() << ' ' << e.what() << '\n';
    }
    return pr;
}

// check to see if the position exists in the level table. If so,
// up its reference count, otherwise create the new position.
//
// Return the PositionId of the position.
PositionId DatabaseObject::create_position(int level, PositionId src, Move move, PositionPacked& pos)
{
    PositionId id;
    std::stringstream ss;
    // std::lock_guard<std::mutex> lock(ins_mtx);
    sess.sql("START TRANSACTION").execute();
    try
    {
        ss << "INSERT INTO position_" << level
           << "(src,move,gi,pop,hi,lo) VALUES ("
           << uint64_t(src) << ','
           << uint16_t(move.pack().i) << ','
           << uint32_t(pos.gi.i) << ','
           << uint64_t(pos.pop) << ','
           << uint64_t(pos.hi) << ','
           << uint64_t(pos.lo)
           << ");";
        sess.sql(ss.str()).execute();
        mysqlx::RowResult res = sess.sql("SELECT LAST_INSERT_ID();").execute();
        id = res.fetchOne().get(0).get<uint64_t>();
    } catch(mysqlx::abi2::r0::Error e) {
        std::cerr << ss.str() << ' ' << e.what() << '\n';
        // key already exists - so increment reference count
        for (int retryCnt = 0; retryCnt < 4; retryCnt++)
        {
            try
            {
                ss.str(std::string());
                ss << "SELECT id FROM position_" << level
                   << " WHERE gi=" << uint32_t(pos.gi.i)
                   << " AND pop=" << uint64_t(pos.pop)
                   << " AND hi=" << uint64_t(pos.hi)
                   << " AND lo=" << uint64_t(pos.lo)
                   << " FOR UPDATE;";
                mysqlx::RowResult res = sess.sql(ss.str()).execute();
                id = res.fetchOne().get(0).get<uint64_t>();
                ss.str(std::string());
                ss << "UPDATE position_" << level
                   << " SET ref_cnt = ref_cnt + 1 "
                   << "WHERE id = " << id << ";";
                sess.sql(ss.str()).execute();
                break;
            } catch(const std::exception& e) {
                std::cerr << ss.str() << ' ' << e.what() << '\n';
            }
        }
    }
    sess.sql("COMMIT;").execute();

    return id;
}

void DatabaseObject::set_endgame_reason(int level, PositionId id, EndGameReason egr)
{
    std::stringstream ss;
    sess.sql("START TRANSACTION").execute();
    ss << "UPDATE position_" << level << " SET"
       << " endgame="  << egr
       << " WHERE id=" << uint64_t(id)
       << ";";
    sess.sql(ss.str()).execute();
    sess.sql("COMMIT;").execute();
}

void DatabaseObject::set_move_count(int level, PositionId id, int size)
{
    std::stringstream ss;
    sess.sql("START TRANSACTION").execute();
    ss << "UPDATE position_" << level << " SET"
       << " move_cnt="  << int32_t(size)
       << " WHERE id=" << uint64_t(id);
    sess.sql(ss.str()).execute();
    sess.sql("COMMIT;").execute();
}
