#include <stddef.h>
#include <stdint.h>

typedef struct Cache Cache;

Cache* cache_new(const size_t num_sets, const size_t set_size, const size_t line_size, const bool write);

void cache_write(Cache* cache, const uint32_t address);
void cache_read(Cache* cache, const uint32_t address);
void cache_invalidate_all(Cache* cache);
void cache_invalidate_entry(Cache* cache, const uint32_t address);
void cache_free(Cache* cache);

void cache_decode_debug(const Cache* cache, const char* cache_name);
