#include <iostream>
#include "constants.h"
#include "db.h"

std::istream& operator>>(std::istream& is, PositionPacked& p) {
    return is;
}


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
            "gi INT unsigned DEFAULT NULL,"
            "pop BIGINT unsigned DEFAULT NULL,"
            "hi BIGINT unsigned DEFAULT NULL,"
            "lo BIGINT unsigned DEFAULT NULL,"
            "move_cnt SMALLINT DEFAULT 0,"
            "ref_cnt SMALLINT DEFAULT 0,"
            "resolved BOOLEAN DEFAULT FALSE,"
        "PRIMARY KEY (id),"
        "UNIQUE KEY pos_idx (gi,pop,hi,lo) USING BTREE);";
    sess.sql(ss.str()).execute();
}

void DatabaseObject::create_moves_table(int level)
{
    std::stringstream ss;
    ss << "CREATE TABLE IF NOT EXISTS "
        "move_" << level << "("
            "src BIGINT unsigned NOT NULL,"
            "move TINYINT unsigned DEFAULT NULL,"
            "trg BIGINT unsigned DEFAULT NULL,"
        "UNIQUE KEY mov_idx (src,move) USING BTREE);";
    sess.sql(ss.str()).execute();
}

PositionRecord DatabaseObject::get_next_unresolved_position(int level)
{
    PositionRecord pr;
    std::stringstream ss;
    sess.sql("START TRANSACTION").execute();
    ss << "SELECT id, gi, pop, hi, lo FROM position_" << level << " WHERE resolved=false LIMIT 1 FOR UPDATE;";
    mysqlx::RowResult res = sess.sql(ss.str()).execute();
    if (res.count() > 0) {
        mysqlx::Row row = res.fetchOne();
        pr.id     = row.get(0).get<uint64_t>();
        pr.pp.gi  = row.get(1).get<uint32_t>();
        pr.pp.pop = row.get(2).get<uint64_t>();
        pr.pp.hi  = row.get(3).get<uint64_t>();
        pr.pp.lo  = row.get(4).get<uint64_t>();
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

// check to see if the position exists in the level table. If so,
// up its reference count, otherwise create the new position.
//
// Return the PositionId of the position.
PositionId DatabaseObject::create_position(int level, PositionPacked& pos)
{
    PositionId id;
    std::stringstream ss;
    sess.sql("START TRANSACTION").execute();
    ss << "SELECT id FROM position_" << level << " WHERE gi=" << uint32_t(pos.gi.i)
       << " AND pop=" << uint64_t(pos.pop)
       << " AND hi=" << uint64_t(pos.hi)
       << " AND lo=" << uint64_t(pos.lo)
       << " FOR UPDATE;";
    mysqlx::RowResult res = sess.sql(ss.str()).execute();
    if (res.count() > 0) {
        id = res.fetchOne().get(0).get<uint64_t>();

        ss.str(std::string());
        ss << "UPDATE position_" << level << " SET ref_cnt = ref_cnt + 1 WHERE id = " << id << ";";
        sess.sql(ss.str()).execute();
    } else {
        ss.str(std::string());
        ss << "INSERT INTO position_" << level << "(gi,pop,hi,lo) VALUES ("
           << uint32_t(pos.gi.i) << ','
           << uint64_t(pos.pop) << ','
           << uint64_t(pos.hi) << ','
           << uint64_t(pos.lo) << ");";
        std::string sql(ss.str());
        sess.sql(sql).execute();
        mysqlx::RowResult res = sess.sql("SELECT LAST_INSERT_ID();").execute();
        id = res.fetchOne().get(0).get<uint64_t>();
    }
    sess.sql("COMMIT;").execute();

    return id;
}

void DatabaseObject::update_position(int level, PositionRecord& rec)
{
    std::stringstream ss;
    sess.sql("START TRANSACTION").execute();
    ss << "SELECT id FROM position_" << level << " WHERE id=" << uint64_t(rec.id) << " FOR UPDATE";
    sess.sql(ss.str()).execute();
    ss.str(std::string());
    ss << "UPDATE position_" << level << " SET"
       <<  " gi="  << uint32_t(rec.pp.gi.i)
       << ", pop=" << uint64_t(rec.pp.pop)
       << ", hi="  << uint64_t(rec.pp.hi)
       << ", lo="  << uint64_t(rec.pp.lo)
       << " WHERE id=" << uint64_t(rec.id);
    sess.sql(ss.str()).execute();
    sess.sql("COMMIT;").execute();
}

void DatabaseObject::set_move_count(int level, PositionId id, int size)
{
    std::stringstream ss;
    sess.sql("START TRANSACTION").execute();
    ss << "SELECT id FROM position_" << level << " WHERE id=" << uint64_t(id) << " FOR UPDATE";
    sess.sql(ss.str()).execute();
    ss.str(std::string());
    ss << "UPDATE position_" << level << " SET"
       <<  " move_cnt="  << int32_t(size)
       << " WHERE id=" << uint64_t(id);
    sess.sql(ss.str()).execute();
    sess.sql("COMMIT;").execute();

}

// check to see if the position exists in the level table. If so,
// up its reference count, otherwise create the new position.
//
// Return the PositionId of the position.
void DatabaseObject::create_move(int level, PositionId src, Move move, PositionId trg)
{
    PositionId id;
    std::stringstream ss;
    sess.sql("START TRANSACTION").execute();
    ss << "INSERT INTO move_" << level << "(src,move,trg) VALUES ("
        << uint64_t(src) << ','
        << uint8_t(move.pack().b) << ','
        << uint64_t(trg);
    sess.sql(ss.str()).execute();
    sess.sql("COMMIT;").execute();
}