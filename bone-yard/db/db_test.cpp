#include <iostream>
#include <db_cxx.h>

#pragma pack(1)

struct Key
{
    u_int32_t   gi;
    u_int64_t   pop;
    u_int64_t   hi;
    u_int64_t   lo;
};

struct Data
{
    u_int64_t      id;        // unique id for this position
    u_int64_t      src;       // the parent of this position
    u_int32_t      move;      // the Move that created in this position
    short          move_cnt;  // number of valid moves for this position
    short          distance;  // number of moves from the initial position
    short          fifty_cnt; // number of moves since last capture or pawn move (50-move rule [14F])
    short          egr;       // end game reason
};

#pragma pack()

int compare_key(Db *dbp, const Dbt *a, const Dbt *b, size_t *locp)
{
    Key& l = *(static_cast<Key*>(a->get_data()));
    Key& r = *(static_cast<Key*>(a->get_data()));

    if (l.pop == r.pop)
    {
        if (l.hi == r.hi)
        {
            if (l.lo == r.lo)
            {
                return l.gi - r.gi;
            }
            return l.lo - r.lo;
        }
        return l.hi - r.hi;
    }
    return l.pop - r.pop;
}

Key k;
Data d;

Dbt key(&k, sizeof(Key));
Dbt data(&d, sizeof(Data));

Db db(NULL,0);
u_int32_t oFlags = DB_CREATE;

int main()
{
    try
    {
        // db.set_bt_compare(compare_key);
        db.open(nullptr,
            "/mnt/c/tmp/my_db.db",
            nullptr,
            DB_HASH,
            oFlags,
            0
        );

        u_int64_t i(0);
        for(; i < 1000; i++)
        {
            k.gi++;
            k.pop++;
            k.hi++;
            k.lo++;
            d.id++;
            int ret = db.put(NULL, &key, &data, DB_NOOVERWRITE);
            if ( ret == DB_KEYEXIST )
                std::cout << "key exists - oops" << std::endl;
            if ((i % 10) == 0)
                std::cout << i << '\r';
        }
        std::cout << i << std::endl;

        k.gi = k.pop = k.hi = k.lo = 500;
        key.set_data(&k);
        key.set_size(sizeof(Key));
        data.set_data(&d);
        data.set_ulen(sizeof(Data));
        data.set_flags(DB_DBT_USERMEM);
        int ret = db.get(NULL, &key, &data, 0);

        db.close(0);
    } catch(DbException &e) {
        std::cout << e.get_errno() << std::endl;
    } catch(std::exception &e) {
        std::cout << e.what() << std::endl;
    }
    return 0;
}