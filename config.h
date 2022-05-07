#pragma once

#define THREAD_COUNT 1

#define WORK_FILE_PATH "/home/codefool/work/data/"

// 50-move rule
//#define ENFORCE_14F_50_MOVE_RULE

// uncomment to cache resolved positions
//#define CACHE_RESOLVED_POSITIONS

// uncomment to cache pawn-move positions rather than shunt to files
//#define CACHE_PAWN_MOVE_POSITIONS

// uncomment to cache n-1 positions rather than shunt to files
//#define CACHE_N_1_POSITIONS

// uncomment to segregate pawn moves
// #define SEGREGATE_PAWN_MOVES

#define USE_DISK_QUEUE
