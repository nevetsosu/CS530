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
  WritePolicy write_policy;
  WriteMissPolicy write_miss_policy;

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
  static const char format_str[] = "\t%-20s %10s\n";

  printf("Printing Cache: %s\n", cache_name ? cache_name : "");
  printf(format_num, "num_sets", cache->num_sets);
  printf(format_num, "Set_size", cache->set_size);
  printf(format_num, "line_size", cache->line_size);
  printf(format_str, "write policy", cache->write_policy == WRITE_THROUGH ? "write_through" : "write_back");
  printf(format_str, "write miss policy", cache->write_miss_policy == WRALLOC ? "write allocate" : "no write allocate");
  
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
Cache* cache_new(const size_t num_sets, const size_t set_size, const size_t line_size, const WritePolicy write_policy, const WriteMissPolicy write_miss_policy) {
  Cache* cache = malloc(sizeof(Cache));
  cache->stats = calloc(1, sizeof(CacheStats));
  cache->sets = malloc(sizeof(Set*) * num_sets);
  
  // configure sets and cache entries 
  for (size_t i = 0; i < num_sets; i++) {
    Set* set = cache->sets[i] = Set_new(set_size);
    
    // sentinel will also be assigned a CacheEntry (though itll never be used)
    // This makes it easier to handle a continous CacheEntry array
    CacheEntry* entries = calloc(set_size + 1, sizeof(CacheEntry));
    SetNode* node_list = set->node_list;
    
    // connect set data to cache entries
    for (size_t j = 0; j < set_size + 1; j++) {
      node_list[j].data = (void*)(entries + j);
    }

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
  cache->write_policy = write_policy;
  cache->write_miss_policy = write_miss_policy;
  cache->next = cache->prev = NULL;

  cache_invalidate_all(cache);

  return cache;
}

CacheStats* cache_stats(const Cache* cache) {
  return cache->stats;
}

bool _cache_find(const Cache* cache, const uint32_t tag, const uint32_t index, SetNode** node) {
  Set* set = cache->sets[index];

  SetNode* cur;
  SET_TRAVERSE_RIGHT(cur, set->node_list) {
    CacheEntry* entry = (CacheEntry*) cur->data;
    if (entry->tag != tag || !entry->valid) continue;

    *node = cur;
    return true;
  }

  return false;
}

void _cache_writeback(Cache* cache, const uint32_t address, bool update_lru) {
  fprintf(stderr, "write back address %x\n", address);
  if (cache->next) {
    cache_write(cache->next, address, update_lru);
  } else {
    cache->stats->mem_accesses += 1;
  }
}

void _cache_readback(Cache* cache, const uint32_t address) {
  if (cache->next)
    cache_read(cache->next, address);
  else
    cache->stats->mem_accesses += 1;
}

static inline uint32_t _cache_address_from_tag_index(const Cache* cache, uint32_t tag, uint32_t index) {
  index = (index & cache->decode.index_mask) << cache->decode.index_pos;
  tag = (tag & cache->decode.tag_mask) << (cache->decode.tag_pos); 
  return tag | index; 
}

// address low and address high will have their index bits ignored
// address_high is INclusive to avoid unsigned overflow
void cache_invalidate_range(Cache* cache, uint32_t address_low, uint32_t address_high) {
  address_low &= ~cache->decode.index_mask;
  address_high &= ~cache->decode.index_mask;

  // propagate the invalidate message up
  if (cache->prev)
    cache_invalidate_range(cache->prev, address_low, address_high);
  
  fprintf(stderr, "invalidating range: %x to %x in cache %s\n", address_low, address_high, cache->stats->name);
  // handle invalidation in the current cache
  for (uint32_t addr = address_low; addr <= address_high; addr += cache->line_size) {
    uint32_t tag, index;
    SetNode* cur;

    _cache_decode(cache, addr, &tag, &index);
    
    // invalidate the entry if found in the set
    Set* set = cache->sets[index];
    SET_TRAVERSE_RIGHT(cur, set->node_list) {
      CacheEntry* entry = (CacheEntry*) cur->data;
      if (!entry->valid || entry->tag != tag) continue;
      
      // invalidate current entry
      entry->valid = false;
      
      // We shouldn't need to check here if it's write through
      // dirty bits should never be set in write through anyway
      if (!entry->dirty)
        continue;

      // write_back to the previous
      _cache_writeback(cache, addr, false);
    }
  }
}

// invalidates and evicts (if necessary) the LRU
// handles writebacks from evictions and upward invalidate propagations
// returns a node that have its cache entry replaced(BUT IS STILL IN THE LRU POSITION)
SetNode* _cache_evict(Cache* cache, const uint32_t index) {
  Set* set = cache->sets[index];
  SetNode* node = Set_get_lru(set);
  CacheEntry* entry = (CacheEntry*) node->data;

  fprintf(stderr, "[%s]evicting tag %u index %u\n", cache->stats->name, entry->tag, index);

  // don't need to invalidate and write back if non-valid
  if (!entry->valid) return node;
  
  // invalidate current entry
  entry->valid = false;
  
  // address range for upper invalidations 
  uint32_t v_addr_low = (entry->tag & cache->decode.tag_mask) << cache->decode.tag_pos;
  v_addr_low |= (index & cache->decode.index_mask) << cache->decode.index_pos;
  uint32_t v_addr_high = v_addr_low + cache->line_size - 1;
  
  // start with the current cache (does a bit of repeatitive search)
  if (cache->prev)
    cache_invalidate_range(cache->prev, v_addr_low, v_addr_high);

  // flush from current cache if dirty
  if (entry->dirty)
    _cache_writeback(cache, v_addr_low, false);

   
  return node;
}

static bool _cache_find_invalid(Cache* cache, const size_t index, SetNode** node) {
  SetNode* cur;
  SET_TRAVERSE_LEFT(cur, cache->sets[index]->node_list) {
    CacheEntry* entry = (CacheEntry*) cur->data;
    if (entry->valid) continue;
    
    *node = cur;
    return true;
  }

  return false;
}

// inserts cache entry data into the cache
// handles evictions if necessary
// returns whether it was a hit
static bool _cache_insert(Cache* cache, const size_t tag, const size_t index, const CacheEntry* new_entry, const bool update_lru) {
  bool hit = false;
  SetNode* node;

  // if hit return true
  if (_cache_find(cache, tag, index, &node)) {
    hit = true;
    goto L_update_lru;
  }
  
  // find an invalid block to replace
  if (_cache_find_invalid(cache, index, &node)) {
    memcpy((CacheEntry*) node->data, new_entry, sizeof(CacheEntry));
    goto L_update_lru;
  }

  // evict the LRU
  node = _cache_evict(cache, index);
  memcpy((CacheEntry*) node->data, new_entry, sizeof(CacheEntry));

L_update_lru:

  if (update_lru)
    Set_set_mru(cache->sets[index], node);

  return hit;
}


void cache_read(Cache* cache, const uint32_t address) {
  uint32_t tag, index;
  
  _cache_decode(cache, address, &tag, &index);

  fprintf(stderr, "[%s]read at address %x\n", cache->stats->name, address);
  
  CacheEntry entry;
  entry.tag = tag;
  entry.valid = true;
  entry.dirty = false;

  cache->stats->hit = _cache_insert(cache, tag, index, &entry, true);
  if (cache->stats->hit) 
    cache->stats->hits += 1;
  else
    _cache_readback(cache, address);
    
  cache->stats->address = address;
  cache->stats->tag = tag;
  cache->stats->index = index;
  cache->stats->total_accesses += 1;
  cache->stats->type = CACHE_READ;
  cache->stats->show = true;
  cache->stats->reads += 1;
}

void cache_write(Cache* cache, const uint32_t address, bool update_lru) {
  uint32_t tag, index;
  SetNode* node;
  
  _cache_decode(cache, address, &tag, &index);

  fprintf(stderr, "[%s] write at address %x\n", cache->stats->name, address);
 
  cache->stats->total_accesses += 1;
  cache->stats->type = CACHE_WRITE;
  cache->stats->tag = tag;
  cache->stats->index = index;

  Set* set = cache->sets[index];

  // hit
  cache->stats->hit = _cache_find(cache, tag, index, &node);

  if (cache->stats->hit) {
    Set_set_mru(set, node);
   
    // action based on WRITE MODE
    if (cache->write_policy == WRITE_THROUGH)
      _cache_writeback(cache, address, update_lru);
    else {
      CacheEntry* entry = (CacheEntry*) node->data;
      entry->dirty = true;
    }

    // update stats
    cache->stats->hits += 1;
    if (update_lru)
      cache->stats->show = true;

    return;
  }

  // miss
  if (cache->write_miss_policy == NO_WRALLOC) {
    _cache_writeback(cache, address, update_lru);
  } else {
    CacheEntry entry;
    entry.tag = tag;
    entry.valid = true;
    entry.dirty = true;
    
    // this sets MRU for us
    _cache_insert(cache, tag, index, &entry, true);
    _cache_readback(cache, address);
  }
}
