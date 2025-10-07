#pragma once
#include <stddef.h>
#include <stdint.h>
#include <stdbool.h>


enum WritePolicy {
  WRITE_THROUGH, WRITE_BACK
};

enum WriteMissPolicy {
  WRALLOC, NO_WRALLOC
};

enum AccessType {
  CACHE_READ, CACHE_WRITE
};

typedef enum WritePolicy WritePolicy;
typedef enum WriteMissPolicy WriteMissPolicy;
typedef enum AccessType AccessType;

struct CacheStats {
  uint32_t address;
  uint32_t tag;
  uint32_t index;
  bool hit;
  bool show;

  AccessType type;

  size_t hits;
  size_t reads;
  size_t mem_accesses;
  size_t total_accesses;
  char name[10];
};

typedef struct Cache Cache;
typedef struct CacheStats CacheStats;

Cache* cache_new(const size_t num_sets, const size_t set_size, const size_t line_size, const WritePolicy write_policy, WriteMissPolicy write_miss_policy);

void cache_write(Cache* cache, const uint32_t address, bool update_lru);
void cache_read(Cache* cache, const uint32_t address);
void cache_invalidate_all(Cache* cache);
void cache_invalidate_entry(Cache* cache, const uint32_t address);
void cache_free(Cache* cache);
void cache_connect(Cache* prev, Cache* next);

void cache_decode_debug(const Cache* cache, const char* cache_name);
CacheStats* cache_stats(const Cache* cache);

void cache_invalidate_range(Cache* cache, const uint32_t low_addr, const uint32_t high_addr);

