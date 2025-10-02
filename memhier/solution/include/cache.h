#include <stddef.h>
#include <stdint.h>
#include <stdbool.h>

enum AccessType {
  READ, WRITE
};

struct CacheStats {
  uint32_t address;
  uint32_t tag;
  uint32_t index;
  bool hit;
  bool access_next;

  enum AccessType type;

  size_t hits;
  size_t total_accesses;
};

typedef struct Cache Cache;
typedef struct CacheStats CacheStats;

Cache* cache_new(const size_t num_sets, const size_t set_size, const size_t line_size, const bool write);

void cache_write(Cache* cache, const uint32_t address);
void cache_read(Cache* cache, const uint32_t address);
void cache_invalidate_all(Cache* cache);
bool cache_invalidate_entry(Cache* cache, const uint32_t address);
void cache_free(Cache* cache);
void cache_connect(Cache* prev, Cache* next);

void cache_decode_debug(const Cache* cache, const char* cache_name);
CacheStats* cache_stats(const Cache* cache);

