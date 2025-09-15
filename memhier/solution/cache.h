#include <stddef.h>

typedef struct Cache Cache;

Cache* cache_new(const size_t num_sets, const size_t set_size, const size_t line_size, const bool write);
void cache_invalidate(Cache* cache);
void cache_free(Cache* cache);
void cache_decode_debug(const Cache* cache, const char* cache_name);
