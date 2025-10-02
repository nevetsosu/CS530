#include <stdint.h>
#include <stddef.h>
#include <stdbool.h>
#include <stdlib.h>
#include <limits.h>
#include <stdio.h>
#include <string.h>
#include "config_consts.h"
#include "util.h"
#include "cache.h"
#include "set.h"

typedef struct DecodeConstants DecodeConstants;
typedef struct CacheEntry CacheEntry;

struct DecodeConstants {
  size_t index_pos;
  size_t tag_pos;

  uint32_t tag_mask;
  uint32_t index_mask;
  uint32_t offset_mask;
};

struct CacheEntry {
  uint32_t tag;
  bool valid;
  bool dirty;
};

struct Cache {
  // configuration information
  size_t num_sets;
  size_t set_size;
  size_t line_size;
  bool write;

  // Decoding
  DecodeConstants decode;

  // Table
  Set** sets;

  // multi-level cache access
  Cache* next;
  Cache* prev;
  CacheStats* stats;
};

void cache_decode_debug(const Cache* cache, const char* cache_name) {
  static const char format_num[] = "\t%-20s %5lu\n";
  static const char format_bool[] = "\t%-20s %5s\n";

  printf("Printing Cache: %s\n", cache_name ? cache_name : "");
  printf(format_num, "num_sets", cache->num_sets);
  printf(format_num, "Set_size", cache->set_size);
  printf(format_num, "line_size", cache->line_size);
  printf(format_bool, "write", cache->write ? "true" : "false");
  
  printf("\tindex_pos  : %lu\n", cache->decode.index_pos); 
  printf("\ttag_pos    : %lu\n", cache->decode.tag_pos);
  
  printf("\ttag_mask   : ");
  printb(cache->decode.tag_mask);
  printf("\n\tindex_mask : ");
  printb(cache->decode.index_mask); 
  printf("\n\toffset_mask: ");
  printb(cache->decode.offset_mask);
  fputc('\n', stdout);
  
  fflush(stdout);
}

void cache_connect(Cache* prev, Cache* next) {
  prev->next = next;
  next->prev = prev;
}

void _cache_decode(const Cache* cache, const uint32_t address, uint32_t* tag, uint32_t* index) {
  *tag = (address >> cache->decode.tag_pos) & cache->decode.tag_mask;
  *index = (address >> cache->decode.index_pos) & cache->decode.index_mask;
}

// invalidates the entire cache. this isn't concerned with writing back dirty lines.
void cache_invalidate_all(Cache* cache) {
  for (size_t i = 0; i < cache->num_sets; i++) {
    Set* set = cache->sets[i];
    CacheEntry* entries = set->node_list->data;
    memset(entries + 1, 0, sizeof(CacheEntry) * cache->set_size); 
  }
}

void cache_free(Cache* cache) {
  for (size_t i = 0; i < cache->num_sets; i++) {
    Set* set = cache->sets[i];
    free(set->node_list->data);
    Set_free(set);
  }
}

// Assumes proper inputs
Cache* cache_new(const size_t num_sets, const size_t set_size, const size_t line_size, const bool write) {
  Cache* cache = malloc(sizeof(Cache));
  cache->stats = calloc(1, sizeof(CacheStats));
  cache->sets = malloc(sizeof(Set*) * num_sets);
  
  // configure sets and cache entries 
  for (size_t i = 0; i < num_sets; i++) {
    fprintf(stderr, "i: %lu\n", i);
    Set* set = cache->sets[i] = Set_new(set_size);
    
    fprintf(stderr, "to next\n");
    // sentinel will also be assigned a CacheEntry (though itll never be used)
    // This makes it easier to handle a continous CacheEntry array
    CacheEntry* entries = calloc(set_size + 1, sizeof(CacheEntry));
    SetNode* node_list = set->node_list;
    
    // connect set data to cache entries
    for (size_t j = 0; j < set_size + 1; j++) {
      node_list[j].data = (void*)(entries + j);
    }

    fprintf(stderr, "to next\n");
  }

  // fill in decode data
  cache->decode.index_pos = log_2(line_size);
  cache->decode.tag_pos = cache->decode.index_pos + log_2(num_sets);
  cache->decode.offset_mask = ~(~(0u) << cache->decode.index_pos);
  cache->decode.index_mask = ~(~(0u) << (cache->decode.tag_pos - cache->decode.index_pos));
  cache->decode.tag_mask = ~(~(0u) << (32 - cache->decode.tag_pos));
  
  // configuration information
  cache->num_sets = num_sets;
  cache->set_size = set_size;
  cache->line_size = line_size;
  cache->write = write;
  cache->next = cache->prev = NULL;

  cache_invalidate_all(cache);

  return cache;
}

CacheStats* cache_stats(const Cache* cache) {
  return cache->stats;
}

// returns TRUE if tags in the entry and address match, returns FALSE otherwise
static inline bool _cache_tag_match(const uint32_t tag, const CacheEntry* entry) {
  return entry->tag == tag;
}

bool _cache_find(const Cache* cache, const uint32_t tag, const uint32_t index, SetNode** node) {
  Set* set = cache->sets[index];

  SetNode* cur;
  SET_TRAVERSE_RIGHT(cur, set->node_list) {
    CacheEntry* entry = (CacheEntry*) cur->data;
    if (entry->tag != tag) continue;

    *node = cur;
    return true;
  }

  return false;
}

static inline uint32_t _cache_address_from_tag_index(const Cache* cache, uint32_t tag, uint32_t index) {
  index = (index & cache->decode.index_mask) << cache->decode.index_pos;
  tag = (tag & cache->decode.tag_mask) << (cache->decode.tag_pos); 
  return tag | index; 
}

void cache_writeback(Cache* cache, const uint32_t address) {
}

SetNode* _cache_evict(const Cache* cache, const uint32_t index) {
  Set* set = cache->sets[index];
  SetNode* node = Set_get_lru(set);
  CacheEntry* entry = (CacheEntry*) node->data;

  // don't need to invalidate and write back if non-valid
  if (!entry->valid) return node;

  // flush and writeback
    // send a flush signal up
    // for all potential blocks
      // this would require address reconstruction
    
  // write back to cache
  // OR
  // write back to memory
  return node;
}

bool _cache_insert(const Cache* cache, const uint32_t tag, const uint32_t index) {
}

// invalidate a cache entry without writing back if dirty.
bool cache_invalidate_entry(Cache* cache, const uint32_t address) {
  size_t tag, index;
  _cache_decode(cache, address, &tag, &index);
  

}

void cache_read(Cache* cache, const uint32_t address) {
  uint32_t tag, index;
  SetNode* node;

  _cache_decode(cache, address, &tag, &index);

  cache->stats->address = address;
  cache->stats->tag = tag;
  cache->stats->index = index;
  cache->stats->total_accesses += 1;
  cache->stats->access_next = false;
  cache->stats->hit = false;
  cache->stats->type = READ;

  // hit (find) sets MRU
  // miss calls _cache_evict, which may not always end up evicting
  // something (block doesn't need to be written back), but will always return a node that can be replaced
  if (_cache_find(cache, tag, index, &node)) {
    cache->stats->hits += 1;
    cache->stats->hit = true;
  } else {
    node = _cache_evict(cache, index);
    CacheEntry* entry = (CacheEntry*) node->data;
    entry->valid = true;
    entry->dirty = false;
    entry->tag = tag;
  }
  
  Set_set_mru(cache->sets[index], node);
}

void cache_write(Cache* cache, const uint32_t address) {
}
