#include <thread>

#include "dht.h"

namespace cdfl {

const char *p_naught = "\0";

size_t get_thread_hash()
{
    std::hash<std::thread::id> hasher;
    return hasher( std::this_thread::get_id() );
}


} // namespace cdfl