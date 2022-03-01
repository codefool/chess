// CREATE TABLE `position_xx` (
// 	`id` BIGINT NOT NULL AUTO_INCREMENT PRIMARY KEY,
// 	`gi` SMALLINT,
// 	`hi` BIGINT,
// 	`lo` BIGINT,
// 	`move_cnt` SMALLINT DEFAULT 0,
// 	`ref_cnt` SMALLINT DEFAULT 0,
//  `resolved` BOOLEAN DEFAULT FALSE,
// 	UNIQUE KEY `pos_idx` (`gi`,`hi`,`lo`) USING BTREE,
// );

// create table position_31 select id, gi, hi, lo, move_cnt, ref_cnt from position_32;
// create table position_30 select id, gi, hi, lo, move_cnt, ref_cnt from position_32;


// CREATE TABLE `moves_xx` (
// 	`src` BIGINT,
// 	`move` SMALLINT(1),
// 	`trg` BIGINT(1),
// 	KEY `src_idx` (`src`, `move`) USING BTREE
// );

// create table moves_31 select src, move, trg from moves_32;

#pragma once
#include <cstring>
#include <mysqlx/xdevapi.h>
#include "constants.h"

typedef unsigned long long PositionId;

struct PositionRecord
{
    PositionId     id;
    PositionPacked pp;
};

class DatabaseObject
{
private:
      mysqlx::Session sess;

public:
    DatabaseObject(std::string& url);

    void create_position_table(int level);
    void create_moves_table(int level);

    PositionRecord get_next_unresolved_position(int level);
    PositionId create_position(int level, PositionPacked& pos);
    void update_position(int level, PositionRecord& rec);
    void create_move(int level, PositionId src, Move move, PositionId trg);
    void set_move_count(int level, PositionId id, int size);


};
