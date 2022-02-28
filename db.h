// CREATE TABLE `position_xx` (
// 	`id` BIGINT(1) NOT NULL AUTO_INCREMENT,
// 	`gi` SMALLINT(1),
// 	`hi` BIGINT(1),
// 	`lo` BIGINT(1),
// 	`move_cnt` INT(1),
// 	`ref_cnt` INT(1),
// 	UNIQUE KEY `pos` (`gi`,`hi`,`lo`) USING BTREE,
// 	PRIMARY KEY (`id`)
// );

// create table position_31 select id, gi, hi, lo, move_cnt, ref_cnt from position_32;
// create table position_30 select id, gi, hi, lo, move_cnt, ref_cnt from position_32;


// CREATE TABLE `moves_32` (
// 	`src` BIGINT(1),
// 	`move` SMALLINT(1),
// 	`trg` BIGINT(1),
// 	KEY `src_idx` (`src`) USING BTREE
// );

// create table moves_31 select src, move, trg from moves_32;

#pragma once
#include <cstring>
#include <mysqlx/xdevapi.h>

#include "constants.h"
#include "gameinfo.h"
#include "piece.h"

#pragma pack(1)


// how this works
//
// There are 64 squares on the board, so we keep a bitmap where
// each bit represents whether a piece is present or not. The
// index of the bit is the square number, and the cnt of the bit
// is the index into the lo/hi population to identify what the
// piece is.
//
// lo and hi are 64-bit values, mapped into 16 4-bit values that
// identify each piece. Since there can be no more than 32 pieces
// this sufficies for encoding any possible possition.
//
// This is the packed encoding for the initial position:
//
//   uint64_t pop = 0xffff00000000ffff;
//   uint64_t hi  = 0xdcb9abcdeeeeeeee;
//   uint64_t lo  = 0x6666666654312345;
//
// The square numbers (0-63) are represented by the pop value and
// are interpreted left-to-right, so the first bit represent square
// A1 on the board. Another point of confusion is that since there
// can be at most 32 pieces on the board, then there can only be 32
// bits set in this 64 bit map. Also, each piece is encoded as a
// 4-bit value, which is futher packed into two 64-bit values. So,
// as the pop map is traversed, a count of set bits (usually referred
// to as the population of bits) is made, with the ordinal of any
// given bit being the index into the hi/lo packed maps. So even though
// the pop map contains 64 bits, at most its population is 32 bits set.
//
// The lo and hi maps are treated as a single 128-bit value, and more
// specifically as an array of 32 4-bit values that identify each
// specific piece.
//
struct PositionPacked {
    GameInfoPacked gi;
    uint64_t       pop;   // population bitmap
    uint64_t       lo;    // lo 64-bits population info
    uint64_t       hi;    // hi 64-bits population info

    PositionPacked()
    : gi{0}, pop(0), lo(0), hi(0)
    {}

    PositionPacked(uint64_t p, uint64_t l, uint64_t h)
    : gi{0}, pop(p), lo(l), hi(h)
    {}

    bool operator==(const PositionPacked& o) const {
        return gi == o.gi && hi == o.hi && lo == o.lo && pop == o.pop;
    }
    bool operator<(const PositionPacked& o) const {
        return gi < o.gi || pop < o.pop || hi < o.hi || lo < o.lo;
    }
};
#pragma pack()


