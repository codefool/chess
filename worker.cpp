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

namespace dreid {

#pragma pack(1)

struct TierStats
{
    short    distance; // the tier level
    uint64_t move_cnt; // number of legal moves
    uint64_t coll_cnt; // number of position collisions
    uint64_t capt_cnt; // number of capture positions
    uint16_t cm_cnt;   // number of checkmates
    uint16_t sm_cnt;   // number of stalemates

    TierStats()
    : distance(0), move_cnt(0), coll_cnt(0), capt_cnt(0), cm_cnt(0), sm_cnt(0)
    {}

    TierStats(const TierStats& o)
    : distance(o.distance)
    , move_cnt(o.move_cnt)
    , coll_cnt(o.coll_cnt)
    , capt_cnt(o.capt_cnt)
    , cm_cnt(o.cm_cnt)
    , sm_cnt(o.sm_cnt)
    {}

    TierStats& operator+=(const TierStats& o)
    {
        move_cnt += o.move_cnt;
        coll_cnt += o.coll_cnt;
        capt_cnt += o.capt_cnt;
        cm_cnt   += o.cm_cnt;
        sm_cnt   += o.sm_cnt;
        return *this;
    }
};

struct Stats
{
    int               level;
    uint64_t          col_cnt;
    uint64_t          initpos_cnt;
    uint64_t          capt_cnt;
    int               cm_cnt;
    int               sm_cnt;
    short             tier_cnt;
};
#pragma pack()

typedef std::shared_ptr<TierStats> TierStatsPtr;

std::mutex mtx_tier;
std::map<short,TierStatsPtr> tier_stats;

void add_tier_stats(TierStats& add)
{
    std::lock_guard<std::mutex> lock(mtx_tier);
    TierStatsPtr s;
    if ( tier_stats.contains(add.distance) )
    {
        s = tier_stats[add.distance];
        *s += add;
    }
    else
    {
        s = std::make_shared<TierStats>(add);
        tier_stats[add.distance] = s;
    }
}

std::mutex mtx_stats;
Stats stats;

void print_stats()
{
    std::stringstream ss;
    ss.flags(std::ios::hex);
    ss.fill('0');
    auto ow = ss.width(16);
    ss << "Stats " << stats.level
       << std::endl;
    for( auto t : tier_stats)
        ss << "Tier " << t.second->distance
           << ' ' << t.second->move_cnt
           << ' ' << t.second->capt_cnt
           << ' ' << t.second->coll_cnt
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
        for(short i(0); i < stats.tier_cnt; i++)
        {
            TierStats s;
            std::fread(&s, sizeof(TierStats), 1, fp);
            tier_stats[s.distance] = std::make_shared<TierStats>(s);
        }
    }
    else
    {
        std::memset(&stats, 0x00, sizeof(stats));
        stats.level = level;
    }
    std::fclose(fp);
    print_stats();
}

void save_stats_file(std::string fspec)
{
    std::filesystem::path path(fspec);
    bool exists = std::filesystem::exists(path);
    FILE *fp = std::fopen(fspec.c_str(), "w+");
    stats.tier_cnt = tier_stats.size();
    std::fwrite(&stats, sizeof(stats), 1, fp);
    for (auto t : tier_stats)
        fwrite(t.second.get(), sizeof(TierStats), 1, fp);
    std::fclose(fp);
    print_stats();
}

std::mutex mtx_id;
PositionId g_id_cnt = 0;

PositionId get_position_id(int level)
{
    std::lock_guard<std::mutex> lock(mtx_id);
    return PositionIdPacked(level, ++g_id_cnt).get();
}

std::mutex unresolved_mtx;

DiskHashTable dht_resolved;
DiskHashTable dht_resolved_ref;
DiskHashTable dht_pawn_n1;
DiskHashTable dht_pawn_n1_ref;

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

    if (dq_get->size() == 0 && dq_put->size() == 0)
    {
        // start from the beginning
        Position pos;
        pos.init();
        PositionRec pr{pos.pack(), PosInfo(get_position_id(level), PosInfo(), Move().pack())};
        dq_get->push((const dq_data_t)&pr);
    }
    return true;
}

bool get_unresolved(PositionRec& pr)
{
    bool retried = false;
    while (true)
    {
        for (int retry(0); retry < 3; ++retry)
        {
            std::unique_lock<std::mutex> lock(unresolved_mtx);
            if ( dq_get->pop( (dq_data_t)&pr ))
            {
                PosInfo ppi;
                if (dht_resolved.search((ucharptr_c)&pr.pp, (ucharptr_c)&ppi))
                {
                    PosRefRec prr(pr.pi.parent, pr.pi.move, ppi.id);
                    dht_resolved_ref.append((ucharptr_c)&prr);
                    stats.col_cnt++;
                    retry--;
                    continue;
                }
                dht_resolved.insert((ucharptr_c)&pr.pp, (ucharptr_c)&pr.pi);
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

void worker(int level)
{
    std::cout << std::this_thread::get_id() << " starting level " << level << std::endl;
    std::stringstream ss;

    MoveList moves;
    moves.reserve(50);

    int loop_cnt{0};
    int retry_cnt{0};
    while (!stop)
    {
        PositionRec prBase;
        if ( !get_unresolved(prBase) )
            break;

        retry_cnt = 0;

        Board sub_board(prBase.pp);
        Side  s = sub_board.gi().getOnMove();

        moves.clear();
        sub_board.get_all_moves(s, moves);
        TierStats tstats;
        tstats.distance = prBase.pi.distance;

        prBase.pi.move_cnt = moves.size();
        if (moves.size() == 0)
        {
            // no moves - so either checkmate or stalemate
            bool onside_in_check = sub_board.test_for_attack(sub_board.getPosition().get_king_pos(s), s);
            if (onside_in_check)
            {
                prBase.pi.egr = EGR_CHECKMATE;
                stats.cm_cnt++;
                tstats.cm_cnt++;
            }
            else
            {
                prBase.pi.egr = EGR_14A_STALEMATE;
                stats.sm_cnt++;
                tstats.sm_cnt++;
            }
            std::cout << std::this_thread::get_id() << " checkmate/stalemate:" << sub_board.getPosition().fen_string() << std::endl;
        }
        else
        {
            short distance = prBase.pi.distance + 1;
            for (MovePtr mv : moves)
            {
                Board brdPrime(prBase.pp);
                bool isPawnMove = brdPrime.process_move(mv, brdPrime.getPosition().gi().getOnMove());
                // we need to flip the on-move
                brdPrime.getPosition().gi().toggleOnMove();
                PositionRec prPrime
                {
                    brdPrime.getPosition(),
                    PosInfo(get_position_id(level), prBase.pi, mv->pack())
                };
                prPrime.pi.distance = distance;
                PosInfo piFound;

                // 50-move rule: drawn game if no pawn move or capture in the last 50 moves.
                // hence, if this is a pawn move or a capture, reset the counter.
                // if (isPawnMove || mv.getAction() == MV_CAPTURE)
                // {
                //     posinfo.fifty_cnt = 0;  // pawn move - reset 50-move counter
                // }
                if (brdPrime.gi().getPieceCnt() == level-1)
                {
                    stats.capt_cnt++;
                    tstats.capt_cnt++;
                    if ( dht_pawn_n1.search( (ucharptr_c)&prPrime.pp, (ucharptr)&piFound ) )
                    {
                        PosRefRec prr( prBase.pi.id, mv, piFound.id );
                        dht_pawn_n1_ref.append((ucharptr_c)&prr);
                    }
                    else
                    {
                        dht_pawn_n1.append( (ucharptr_c)&prPrime.pp, (ucharptr_c)&prPrime.pi );
                    }
                }
                else if(brdPrime.gi().getPieceCnt() == level)
                {
                    if ( dht_resolved.search( (ucharptr_c)&prPrime.pp, (ucharptr_c)&piFound ) )
                    {
                        PosRefRec prr(prBase.pi.id, mv, piFound.id);
                        dht_resolved_ref.append((ucharptr_c)&prr);
                        stats.col_cnt++;
                        tstats.coll_cnt++;
                    }
                    else
                    {
                        dq_put->push((const dq_data_t)&prPrime);
                        tstats.move_cnt++;
                    }
                }
                else
                {
                    std::cout << "ERROR! too many captures "
                              << brdPrime.gi().getPieceCnt()
                              << ' ' << brdPrime.getPosition().fen_string()
                              << std::endl;
                    // stop = true;
                }
            }   // end for()
        }

        dht_resolved.update((ucharptr_c)&prBase.pp, (ucharptr)&prBase.pi);
        add_tier_stats(tstats);
        // std::cout << "base,parent,mov/p/c/5/1,move,dist,coll_cnt,init_cnt,res_cnt,get,put,unr1,fifty,FEN\n";
        ss.str(std::string());
        ss.flags(std::ios::hex);
        ss.fill('0');
        auto ow = ss.width(16);
        ss  << prBase.pi.id
            << ',' << prBase.pi.parent;
        ss.width(ow);
        ss  << ',' << dq_get->size()
            << ',' << dq_put->size()
            << ',' << stats.col_cnt
            << ',' << Move::unpack(prBase.pi.move)
            << ',' << prBase.pi.distance
            << ',' << stats.capt_cnt
            << ',' << dht_resolved.size()
            // << std::flush;
        // ss.width(2);
            << ',' << std::setw(2) << tstats.move_cnt
            << '/' << tstats.coll_cnt
            << '/' << tstats.capt_cnt
            << ',' << sub_board.getPosition().fen_string(prBase.pi.distance)
            << '\n';
        ss.width(ow);
        std::cout << ss.str();
    }

    ss.str(std::string());
    ss << std::this_thread::get_id() << " stopping\n";
    std::cout << ss.str();
}

} // namespace dreid