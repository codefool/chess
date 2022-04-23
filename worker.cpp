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

std::mutex mtx_id;
PositionId g_id_cnt = 0;

void set_global_id_cnt(PositionId id)
{
  g_id_cnt = id;
}

// a position id is a 64-bit value, with the high
// six bits being the piece population
union PositionIdPacked {
  uint64_t  uul;
  struct {
    uint64_t m:58;
    uint64_t l:6;
  } f;

  PositionIdPacked(int level, PositionId cnt)
  {
      f.m = cnt;
      f.l = level;
  }

  PositionId get()
  {
      return static_cast<uint64_t>(uul);
  }
};

PositionId get_position_id(int level)
{
    std::lock_guard<std::mutex> lock(mtx_id);
    return PositionIdPacked(level, ++g_id_cnt).get();
}

uint64_t collisioncnt = 0;
uint64_t initposcnt = 0;
uint64_t unresolvedn1cnt = 0;
uint64_t fiftymovedrawcnt = 0;
int checkmate = 0;

std::mutex unresolved_mtx;

#ifdef CACHE_RESOLVED_POSITIONS
    std::mutex resolved_mtx;
    PosMap resolved;
#else
    DiskHashTable dht_resolved(WORK_FILE_PATH, "resolved", 32, sizeof(PositionPacked), sizeof(PosInfo));
    DiskHashTable dht_resolved_ref(WORK_FILE_PATH, "resolved_ref", 32, sizeof(PosRefRec));
#endif

#ifdef SEGREGATE_PAWN_MOVES
#   ifdef CACHE_PAWN_MOVE_POSITIONS
        std::mutex pawn_init_pos_mtx;
        PosMap pawn_init_pos;
#   else
        DiskHashTable dht_pawn_init_pos(WORK_FILE_PATH, "pawn_init", 32, sizeof(PositionPacked), sizeof(PosInfo));
#   endif
#endif

#ifndef CACHE_N_1_POSITIONS
    DiskHashTable dht_pawn_n1(WORK_FILE_PATH, "pawn_init", 31, sizeof(PositionPacked), sizeof(PosInfo));
    DiskHashTable dht_pawn_n1_ref(WORK_FILE_PATH, "pawn_init_ref", 31, sizeof(PosRefRec));
#endif

// We process unresolved positions by distance. Since a position of
// distance n can only generate a position of distance n+1, we only
// have to keep two queues. When the queue at distance n depletes,
// we switch to the n+1 queue which becomes the new n, and the
// old queue becomes the n+1 queue.
#ifdef USE_DISK_QUEUE
#   pragma pack(1)
    struct PositionRec
    {
        PositionPacked  pp;
        PosInfo         pi;

        PositionRec() {}
        PositionRec(PositionPacked& p, PosInfo& i)
        : pp(p), pi(i)
        {}
    };
#   pragma pack()
    DiskQueue dq_unr0(WORK_FILE_PATH, "unresolved0", sizeof( PositionRec ));
    DiskQueue dq_unr1(WORK_FILE_PATH, "unresolved1", sizeof( PositionRec ));
    DiskQueue *dq_get = &dq_unr0;
    DiskQueue *dq_put = &dq_unr1;
#else
    PosMap  unresolved0;  // the 'n' queue
    PosMap  unresolved1;  // the 'n+1' queue
    PosMap* get_queue = &unresolved0;    // the 'n' queue
    PosMap* put_queue = &unresolved1;    // the 'n+1' queue
#endif

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
#ifdef USE_DISK_QUEUE
    PositionRec pr(pp,pi);
    dq_put->push( (const dq_data_t)&pr );
#else
    put_queue->insert({pp,pi});
#endif
}

short resolved_min_dist = 9999;
short resolved_max_dist = 0;

bool get_unresolved(PositionPacked& pos, PosInfo& pi, int level, std::string& base_path)
{
    // retried false
    // while true
    //   retry 0
    //   while retry < 3
    //     lock the queues
    //     if get queue empty
    //       release lock
    //       wait 50ms
    //       retry++
    //     else
    //       pop get queue
    //       return true
    //   end while
    //   if retried
    //     return false
    //   lock queues
    //   swap queues
    // end while
    bool retried = false;
    while (true)
    {
        for (int retry(0); retry < 3; ++retry)
        {
            std::unique_lock<std::mutex> lock(unresolved_mtx);
#ifdef USE_DISK_QUEUE
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
                    collisioncnt++;
                    retry--;
                    continue;
                }
                dht_resolved.insert((ucharptr_c)&pos, (ucharptr_c)&pi);
                return true;
            }
#else
            if ( get_queue->size() )
            {
                auto itr = get_queue->begin();
                if (itr != get_queue->end())
                {
                    pos = itr->first;
                    pi  = itr->second;
                    get_queue->erase(pos);
#                   ifdef CACHE_RESOLVED_POSITIONS
                        std::lock_guard<std::mutex> lock1(resolved_mtx);
                        resolved.insert({pos,pi});
                        resolved_min_dist = std::min(resolved_min_dist, pi.distance);
                        resolved_max_dist = std::max(resolved_max_dist, pi.distance);
#                   else
                        dht_resolved.insert((ucharptr_c)&pos, (ucharptr_c)&pi);
#                   endif
                    return true;
                }
            }
#endif
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
#ifdef USE_DISK_QUEUE
            std::swap(dq_get, dq_put);
#else
            // get here if the current queue has run dry.
            // swap the queues
            if (put_queue->size())
                std::swap(get_queue, put_queue);
#endif
            retried = true;
        EndDummyScope

#ifdef CACHE_RESOLVED_POSITIONS
        // check if we have more than 3 tiers of results in the
        // resolved list. If so, spool to extra tiers out
        if ( resolved_max_dist - resolved_min_dist >= 3 )
        {
            resolved_min_dist = resolved_max_dist - 3;
            write_resolved(level, base_path, resolved_min_dist);
        }
#endif
    }
}

void worker(int level, std::string base_path)
{
    std::cout << std::this_thread::get_id() << " starting level " << level << std::endl;

#   ifdef CACHE_PAWN_MOVE_POSITIONS
        PositionFile f_pawninitpos(base_path, "pawn_init_pos", level);
#   endif
#   ifdef CACHE_N_1_POSITIONS
        PositionFile f_downlevel(base_path, "init_pos", level - 1);
#   endif
    std::stringstream ss;

    MoveList moves;
    moves.reserve(50);

    // time_t start = time(0);

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
            base_info.egr = (onside_in_check) ? EGR_CHECKMATE : EGR_14A_STALEMATE;
            checkmate++;
            std::cout << std::this_thread::get_id() << " checkmate/stalemate:" << sub_board.getPosition().fen_string() << std::endl;
        } else {
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

#               ifdef SEGREGATE_PAWN_MOVES
                if (isPawnMove)
                {
                    // new initial position
                    initposcnt++;
                    pawn_moves++;
                    // initpos->insert({posprime,posinfo});
                    posinfo.id = get_position_id(level);
#                   ifdef CACHE_PAWN_MOVE_POSITIONS
                        std::lock_guard<std::mutex> lock(pawn_init_pos_mtx);
                        auto it = pawn_init_pos.find(posprime);
                        if (it == pawn_init_pos.end())
                            pawn_init_pos.insert({posprime,posinfo});
                        else
                            it->second.add_ref(mv, base_info.id);
#                   else
                        //   f_pawninitpos.write(posprime,posinfo);
                        dht_pawn_init_pos.insert((ucharptr_c)&posprime, (ucharptr_c)&posinfo);
#                   endif
                    continue;
                }
#               endif
#ifdef ENFORCE_14F_50_MOVE_RULE
                if (posinfo.fifty_cnt == 50)
                {
                    // if no pawn move or capture in past fifty moves, draw the game
                    base_info.egr = EGR_14F_50_MOVE_RULE;
                    fiftymovedrawcnt++;
                    fifty_cnt++;
                    continue;
                }
#endif
                if (bprime.gi().getPieceCnt() == level-1)
                {
                    unresolvedn1cnt++;
                    posinfo.id = get_position_id(level-1);
#                   ifdef CACHE_N_1_POSITIONS
                        f_downlevel.write(posprime,posinfo);
#                   else
                        PosInfo pi;
                        if (dht_resolved.search((ucharptr_c)&pprime, (ucharptr)&pi))
                        {
                            PosRefRec prr(base_info.id, mv, pi.id);
                            dht_pawn_n1_ref.append((ucharptr_c)&prr);
                        }
                        else
                        {
                            dht_pawn_n1.append((ucharptr_c)&posprime, (ucharptr_c)&posinfo);
                        }
#                   endif
                    nsub1_cnt++;
                }
                else if(bprime.gi().getPieceCnt() == level)
                {
                    bool found = false;
                    BeginDummyScope
#                       ifdef CACHE_RESOLVED_POSITIONS
                            std::lock_guard<std::mutex> lock(resolved_mtx);
                            auto itr = resolved.find(posprime);
                            if (itr != resolved.end()) {
                                itr->second.add_ref(mv, base_info.id);
                                collisioncnt++;
                                coll_cnt++;
                                found = true;
                            }
#                       else
                            PosInfo pi;
                            if (dht_resolved.search((ucharptr_c)&pprime, (ucharptr_c)&pi))
                            {
                                PosRefRec prr(base_info.id, mv, pi.id);
                                dht_resolved_ref.append((ucharptr_c)&prr);
                                collisioncnt++;
                                coll_cnt++;
                                found = true;
                            }
#                       endif
                    EndDummyScope
                    if (!found)
                    {
                        std::lock_guard<std::mutex> lock(unresolved_mtx);
#ifdef USE_DISK_QUEUE
                        posinfo.id = get_position_id(level);
                        PositionRec pr(posprime, posinfo);
                        dq_put->push((const dq_data_t)&pr);
#else
                        auto itr = put_queue->find(posprime);
                        if ( itr != put_queue->end() ) {
                            itr->second.add_ref(mv, base_info.id);
                            collisioncnt++;
                            coll_cnt++;
                        }
                        else
                        {
                            posinfo.id = get_position_id(level);
                            put_queue->insert({posprime, posinfo});
                        }
#endif
                    }
                }
                else
                {
                    std::cout << "ERROR! too many captures " << bprime.gi().getPieceCnt() << std::endl;
                    exit(1);
                }
            }   // end for()

#       ifdef CACHE_RESOLVED_POSITIONS
        BeginDummyScope
            std::lock_guard<std::mutex> lock(resolved_mtx);
            resolved[base_pos] = base_info;
        EndDummyScope
#       else
            dht_resolved.update((ucharptr_c)&base_pos, (ucharptr)&base_info);
#ifdef POSINFO_HAS_REFS
            if (base_info.refs != nullptr && base_info.refs->size() > 0)
            {
                for(PosRef r : *(base_info.refs))
                {
                    PosRefRec prr(base_info.id, r.move, r.trg);
                    dht_resolved_ref.append((ucharptr_c)&prr);
                }
            }
#endif
#       endif

        // std::cout << "base,parent,mov/p/c/5/1,move,dist,dis50,coll_cnt,init_cnt,res_cnt,get,put,unr1,fifty,FEN\n";
        ss.str(std::string());
        ss.flags(std::ios::hex);
        ss.fill('0');
        auto ow = ss.width(16);
        ss  << base_info.id
            << ',' << base_info.src;
        ss.width(ow);
#ifdef USE_DISK_QUEUE
        ss  << ',' << dq_get->size()
            << ',' << dq_put->size()
#else
        ss  << ',' << get_queue->size()
            << ',' << put_queue->size()
#endif
            << ',' << collisioncnt;
        ss  << ',' << Move::unpack(base_info.move)
            << ',' << base_info.distance
            << ',' << unresolvedn1cnt;
#       ifdef ENFORCE_14F_50_MOVE_RULE
            ss  << ',' << base_info.fifty_cnt
                << ',' << fiftymovedrawcnt;
#       endif
#       ifdef SEGREGATE_PAWN_MOVES
#           ifdef CACHE_PAWN_MOVE_POSITIONS
                ss  << ',' << pawn_init_pos.size();
#           else
                ss  << ',' << dht_pawn_init_pos.size();
#           endif
#       endif
#       ifdef CACHE_RESOLVED_POSITIONS
            ss  << ',' << resolved.size();
#       else
            ss  << ',' << dht_resolved.size();
#       endif
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
    // dump_map(initpos);
    // dump_map(unresolved);
    // dump_map(resolved);
  }

  // // if stop, then ctrl-c was used, so bail
  // if (!stop) {
  //   std::cout << std::this_thread::get_id() << " writing resolved positions" << std::endl;
  //   PositionFile f_resolved(base_path, "resolved", level);
  //   for (auto e : resolved)
  //     f_resolved.write(e.first, e.second);

  //   // time_t stop = time(0);
  //   // double hang = std::difftime(stop, start);

  //   // std::cout << std::this_thread::get_id() << '\n'
  //   //           << std::asctime(std::localtime(&start))
  //   //           << std::asctime(std::localtime(&stop))
  //   //           << ' ' << hang << '(' << (hang/60.0) << ") secs"
  //   //           << std::endl;
  // }
    ss.str(std::string());
    ss << std::this_thread::get_id() << " stopping\n";
    std::cout << ss.str();
}

#ifdef CACHE_RESOLVED_POSITIONS
void write_resolved(int level, std::string& base_path, int max_distance)
{
    static bool first = true;
    std::lock_guard<std::mutex> lock1(resolved_mtx);
    if (!max_distance)
        write_results(resolved, level, base_path, "resolved", false);
    else
    {
        std::cout << "Spooling resolved positions up to distance " << max_distance << std::endl;
        PositionFile f_results(base_path, "resolved", level, false, first);
        int cnt(0);
        for (auto itr = resolved.begin(); itr != resolved.end();)
            if (itr->second.distance < max_distance)
            {
                PositionPacked p = itr->first;
                f_results.write(p, itr->second);
                ++itr;
                resolved.erase(p);
                cnt++;
            } else {
                ++itr;
            }
        std::cout << "\tSpooled " << cnt << " resvoled positions" << std::endl;
    }
    first = false;
}
#endif

#ifdef CACHE_PAWN_MOVE_POSITIONS
void write_pawn_init_pos(int level, std::string& base_path)
{
    write_results(pawn_init_pos, level, base_path, "pawn_init_pos");
}
#endif

void write_results(PosMap& map, int level, std::string& base_path, std::string disp_name, bool use_thread_id)
{
    std::cout << " writing " << map.size() << ' ' << disp_name << " positions" << std::endl;
    PositionFile f_results(base_path, disp_name, level, use_thread_id);
    for (auto e : map)
      f_results.write(e.first, e.second);
}