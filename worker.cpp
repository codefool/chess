#include <iostream>
#include <algorithm>
#include <filesystem>
#include <fstream>
#include <mutex>
#include <signal.h>
#include <sstream>
#include <thread>

#include <stxxl/algorithm>

#include "worker.h"

#pragma pack(1)
// a position id is a 64-bit value, with the high
// six bits being the piece population
union PositionIdPacked {
    PositionId  uul;
    struct {
        PositionId m:58;
        PositionId l:6;
    } f;

    PositionIdPacked()
    {}

    PositionIdPacked(int level, PositionId cnt)
    {
        set(level, cnt);
    }

    void set(PositionId id)
    {
        uul = id;
    }

    void set(int level, PositionId cnt)
    {
        f.m = cnt;
        f.l = level;
    }

    PositionId get()
    {
        return static_cast<PositionId>(uul);
    }

    PositionId get_next()
    {
        f.m++;
        return get();
    }
};

struct Stats
{
    int               level;
    PositionIdPacked  first_id;
    PositionIdPacked  last_id;
    uint64_t          col_cnt;
    uint64_t          initpos_cnt;
    uint64_t          unr_n1_cnt;
    int               cm_cnt;
    int               sm_cnt;
};
#pragma pack()

std::mutex mtx_stats;
Stats stats;

void print_stats()
{
    std::stringstream ss;
    ss.flags(std::ios::hex);
    ss.fill('0');
    auto ow = ss.width(16);
    ss << "Stats " << stats.level
       << ' ' << stats.first_id.get()
       << ' ' << stats.last_id.get()
       << std::endl;
    std::cout << ss.str();
}

void load_stats_file(int level, std::string fspec)
{
    std::filesystem::path path(fspec);
    bool exists = std::filesystem::exists(path);
    const char *mode = (exists) ? "r+" : "w+";
    FILE *fp = std::fopen(fspec.c_str(), mode);
    if (exists)
    {
        std::fread(&stats, sizeof(stats), 1, fp);
    }
    else
    {
        std::memset(&stats, 0x00, sizeof(stats));
        stats.level = level;
        stats.first_id.set(level, 0);
        stats.last_id .set(stats.first_id.get());
    }
    std::fclose(fp);
    print_stats();
}

void save_stats_file(std::string fspec)
{
    std::filesystem::path path(fspec);
    bool exists = std::filesystem::exists(path);
    FILE *fp = std::fopen(fspec.c_str(), "w+");
    std::fwrite(&stats, sizeof(stats), 1, fp);
    std::fclose(fp);
    print_stats();
}

PositionId get_position_id(int level)
{
    std::lock_guard<std::mutex> lock(mtx_stats);
    return stats.last_id.get_next();
}

std::mutex unresolved_mtx;

DiskHashTable dht_resolved;
DiskHashTable dht_resolved_ref;
DiskHashTable dht_pawn_n1;
DiskHashTable dht_pawn_n1_ref;

// We process unresolved positions by distance. Since a position of
// distance n can only generate a position of distance n+1, we only
// have to keep two queues. When the queue at distance n depletes,
// we switch to the n+1 queue which becomes the new n, and the
// old queue becomes the n+1 queue.
#pragma pack(1)
struct PositionRec
{
    PositionPacked  pp;
    PosInfo         pi;

    PositionRec() {}
    PositionRec(PositionPacked& p, PosInfo& i)
    : pp(p), pi(i)
    {}
};
#pragma pack()

DiskQueue dq_unr0(WORK_FILE_PATH, "unresolved0", sizeof( PositionRec ));
DiskQueue dq_unr1(WORK_FILE_PATH, "unresolved1", sizeof( PositionRec ));
DiskQueue *dq_get = &dq_unr0;
DiskQueue *dq_put = &dq_unr1;

bool stop = false;    // global halt flag

void ctrl_c_handler(int s) {
  stop = true;
}

void set_stop_handler()
{
  struct sigaction sigIntHandler;

  sigIntHandler.sa_handler = ctrl_c_handler;
  sigemptyset(&sigIntHandler.sa_mask);
  sigIntHandler.sa_flags = 0;

  sigaction(SIGINT, &sigIntHandler, NULL);
}

void insert_unresolved(PositionPacked& pp, PosInfo& pi)
{
    std::lock_guard<std::mutex> lock(unresolved_mtx);
    PositionRec pr(pp,pi);
    dq_put->push( (const dq_data_t)&pr );
}

bool open_tables(int level)
{
    dht_resolved    .open(WORK_FILE_PATH, "resolved", level, sizeof(PositionPacked), sizeof(PosInfo));
    dht_resolved_ref.open(WORK_FILE_PATH, "resolved_ref", level, sizeof(PosRefRec));
    dht_pawn_n1     .open(WORK_FILE_PATH, "pawn_init", level - 1 , sizeof(PositionPacked), sizeof(PosInfo));
    dht_pawn_n1_ref .open(WORK_FILE_PATH, "pawn_init_ref", level - 1, sizeof(PosRefRec));

    if (dq_get->size() == 0)
    {
        // start from the beginning
        Position pos;
        pos.init();
        PositionPacked pp = pos.pack();
        PosInfo pi(get_position_id(level), PosInfo(), Move().pack());
        // this should be put into initpos, but for now
        PositionRec pr(pp, pi);
        dq_get->push((const dq_data_t)&pr);
    }
    return true;
}

bool get_unresolved(PositionPacked& pos, PosInfo& pi, int level, std::string& base_path)
{
    bool retried = false;
    while (true)
    {
        for (int retry(0); retry < 3; ++retry)
        {
            std::unique_lock<std::mutex> lock(unresolved_mtx);
            PositionRec pr;
            if ( dq_get->pop( (dq_data_t)&pr ))
            {
                pos = pr.pp;
                pi  = pr.pi;
                PosInfo pii;
                if (dht_resolved.search((ucharptr_c)&pos, (ucharptr_c)&pii))
                {
                    PosRefRec prr(pi.src, pi.move, pii.id);
                    dht_resolved_ref.append((ucharptr_c)&prr);
                    stats.col_cnt++;
                    retry--;
                    continue;
                }
                dht_resolved.insert((ucharptr_c)&pos, (ucharptr_c)&pi);
                return true;
            }
            else
            {
                lock.unlock();
                using namespace std::chrono_literals;
                std::this_thread::sleep_for(50ms);
            }
        }

        if (retried)
        {
            // if we've already swapped the queues, then there's
            // nothing to do at all!
            std::cout << std::this_thread::get_id() << " both queues empty" << std::endl;
            return false;
        }

        BeginDummyScope
            std::unique_lock<std::mutex> lock(unresolved_mtx);
            std::swap(dq_get, dq_put);
            retried = true;
        EndDummyScope
    }
}

void worker(int level, std::string base_path)
{
    std::cout << std::this_thread::get_id() << " starting level " << level << std::endl;
    std::stringstream ss;

    MoveList moves;
    moves.reserve(50);

    int loop_cnt{0};
    int retry_cnt{0};
    while (!stop)
    {
        PositionPacked base_pos;
        PosInfo        base_info;
        if ( !get_unresolved(base_pos, base_info, level, base_path) )
            break;

        retry_cnt = 0;

        Board sub_board(base_pos);
        Side  s = sub_board.gi().getOnMove();

        moves.clear();
        sub_board.get_all_moves(s, moves);
        int pawn_moves{0};
        int coll_cnt{0};
        int fifty_cnt{0};
        int nsub1_cnt{0};

        if (moves.size() == 0)
        {
            // no moves - so either checkmate or stalemate
            bool onside_in_check = sub_board.test_for_attack(sub_board.getPosition().get_king_pos(s), s);
            if (onside_in_check)
            {
                base_info.egr = EGR_CHECKMATE;
                stats.cm_cnt++;
            }
            else
            {
                base_info.egr = EGR_14A_STALEMATE;
                stats.sm_cnt++;
            }
            std::cout << std::this_thread::get_id() << " checkmate/stalemate:" << sub_board.getPosition().fen_string() << std::endl;
        }
        else
        {
            base_info.move_cnt = moves.size();

            for (Move mv : moves)
            {
                Board bprime(base_pos);
                bool isPawnMove = bprime.process_move(mv, bprime.gi().getOnMove());
                // we need to flip the on-move
                Position pprime = bprime.getPosition();
                pprime.gi().toggleOnMove();
                // std::cout << std::this_thread::get_id() << ' ' << pprime.fen_string() << std::endl;
                PositionPacked posprime = pprime.pack();
                // PosInfo posinfo(get_position_id(), base_info, mv.pack());
                PosInfo posinfo(0, base_info, mv.pack());

                // 50-move rule: drawn game if no pawn move or capture in the last 50 moves.
                // hence, if this is a pawn move or a capture, reset the counter.
                if (isPawnMove || mv.getAction() == MV_CAPTURE)
                {
                    posinfo.fifty_cnt = 0;  // pawn move - reset 50-move counter
                }
                if (bprime.gi().getPieceCnt() == level-1)
                {
                    stats.unr_n1_cnt++;
                    posinfo.id = get_position_id(level-1);
                    PosInfo pi;
                    if (dht_pawn_n1.search((ucharptr_c)&pprime, (ucharptr)&pi))
                    {
                        PosRefRec prr(base_info.id, mv, pi.id);
                        dht_pawn_n1_ref.append((ucharptr_c)&prr);
                    }
                    else
                    {
                        dht_pawn_n1.append((ucharptr_c)&posprime, (ucharptr_c)&posinfo);
                    }
                    nsub1_cnt++;
                }
                else if(bprime.gi().getPieceCnt() == level)
                {
                    bool found = false;
                    BeginDummyScope
                        PosInfo pi;
                        if (dht_resolved.search((ucharptr_c)&pprime, (ucharptr_c)&pi))
                        {
                            PosRefRec prr(base_info.id, mv, pi.id);
                            dht_resolved_ref.append((ucharptr_c)&prr);
                            stats.col_cnt++;
                            coll_cnt++;
                            found = true;
                        }
                    EndDummyScope
                    if (!found)
                    {
                        std::lock_guard<std::mutex> lock(unresolved_mtx);
                        posinfo.id = get_position_id(level);
                        PositionRec pr(posprime, posinfo);
                        dq_put->push((const dq_data_t)&pr);
                    }
                }
                else
                {
                    std::cout << "ERROR! too many captures "
                              << bprime.gi().getPieceCnt()
                              << ' ' << bprime.getPosition().fen_string()
                              << std::endl;
                    // stop = true;
                }
            }   // end for()
        }

        dht_resolved.update((ucharptr_c)&base_pos, (ucharptr)&base_info);
        // std::cout << "base,parent,mov/p/c/5/1,move,dist,dis50,coll_cnt,init_cnt,res_cnt,get,put,unr1,fifty,FEN\n";
        ss.str(std::string());
        ss.flags(std::ios::hex);
        ss.fill('0');
        auto ow = ss.width(16);
        ss  << base_info.id
            << ',' << base_info.src;
        ss.width(ow);
        ss  << ',' << dq_get->size()
            << ',' << dq_put->size()
            << ',' << stats.col_cnt
            << ',' << Move::unpack(base_info.move)
            << ',' << base_info.distance
            << ',' << stats.unr_n1_cnt
            << ',' << dht_resolved.size();
        ss.width(2);
        ss  << ',' << moves.size()
            << '/' << pawn_moves
            << '/' << coll_cnt
            << '/' << fifty_cnt
            << '/' << nsub1_cnt
            << ',' << sub_board.getPosition().fen_string(base_info.distance)
            << '\n';
        std::cout << ss.str();
    }

    ss.str(std::string());
    ss << std::this_thread::get_id() << " stopping\n";
    std::cout << ss.str();
}
