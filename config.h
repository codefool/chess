#pragma once

#define THREAD_COUNT 12

#define WORK_FILE_PATH "/home/codefool/work/data/"

// [8A3] castling permanently illegal if the king moves, or the castling
// rook has moved.
//#define ENFORCE_8A3_CASTLING

// 50-move rule
//#define ENFORCE_14F_50_MOVE_RULE

// uncomment to cache resolved positions
//#define CACHE_RESOLVED_POSITIONS

// uncomment to cache pawn-move positions rather than shunt to files
//#define CACHE_PAWN_MOVE_POSITIONS

// uncomment to cache n-1 positions rather than shunt to files
//#define CACHE_N_1_POSITIONS

// uncomment to enable ref management in PosInfo
//#define POSINFO_HAS_REFS

// uncomment to segregate pawn moves
// #define SEGREGATE_PAWN_MOVES

#define USE_DISK_QUEUE
