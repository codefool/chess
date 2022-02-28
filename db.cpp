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
    ss << "CREATE TABLE position_" << level << "("
        "id BIGINT unsigned NOT NULL AUTO_INCREMENT,"
        "gi INT unsigned DEFAULT NULL,"
        "pop BIGINT unsigned DEFAULT NULL,"
        "hi BIGINT unsigned DEFAULT NULL,"
        "lo BIGINT unsigned DEFAULT NULL,"
        "move_cnt SMALLINT DEFAULT 0,"
        "ref_cnt SMALLINT DEFAULT 0,"
        "resolved BOOLEAN DEFAULT FALSE,"
    "PRIMARY KEY (id),"
    "UNIQUE KEY pos_idx (gi,hi,lo) USING BTREE);";
    sess.sql(ss.str()).execute();
}

void DatabaseObject::create_moves_table(int level)
{
    std::stringstream ss;
    ss << "CREATE TABLE move_" << level << "("
        "src BIGINT unsigned NOT NULL,"
        "move SMALLINT DEFAULT NULL,"
        "trg BIGINT unsigned DEFAULT NULL,"
    "UNIQUE KEY mov_idx (src,move) USING BTREE);";
    sess.sql(ss.str()).execute();
}

PositionId DatabaseObject::get_next_unresolved_position(int level)
{
    std::stringstream ss;
    sess.sql("START TRANSACTION").execute();
    ss << "SELECT id FROM position_" << level << " WHERE resolved=false LIMIT 1 FOR UPDATE;";
    mysqlx::RowResult res = sess.sql(ss.str()).execute();
    if (res.count() > 0) {
        PositionId id = res.fetchOne().get(0).get<uint64_t>();

        ss.str(std::string());
        ss << "UPDATE position_" << level << " SET resolved = true;";
        sess.sql(ss.str()).execute();
        sess.sql("COMMIT;").execute();
        return id;
    } else {
        sess.sql("ROLLBACK;").execute();
    }
    return 0;
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
        ss << "UPDATE position_" << level << " SET ref_cnt = ref_cnt + 1;";
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
        res = sess.sql("SELECT LAST_INSERT_ID();").execute();
        id = res.fetchOne().get(0).get<uint64_t>();
    }
    sess.sql("COMMIT;").execute();

    return 0;
}