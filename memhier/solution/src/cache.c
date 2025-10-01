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

typedef uint64_t CacheEntry;
typedef int CacheMode;

enum CacheMode {
  FULLY_ASSOCIATIVE, DIRECT_MAPPED, SET_ASSOCIATIVE
};

#define CACHE_ENTRY_BITSIZE (sizeof(CacheEntry) * 8)

struct Cache {
  // operation mode
  CacheMode mode;

  // configuration information
  size_t num_sets;
  size_t set_size;
  size_t line_size;
  bool   write;

  // address decoding information
  // offset_pos is assumed to be 0
  size_t offset_mask;
  size_t index_pos;
  size_t index_mask;
  size_t addr_tag_pos;    // the addr tag is the rest of the upper part of the address

  // table entry decoding information
  size_t dirty_pos;
  size_t valid_pos;
  size_t table_tag_pos;

  // shared addr and table
  size_t tag_mask;
  
  // 'hardware' storage
  size_t table_num_frames;
  CacheEntry* table;

  // multi-level cache access
  Cache* next;
  Cache* prev;
  CacheStats* stats;
};

void cache_decode_debug(const Cache* cache, const char* cache_name) {
  static const char* mode_str[] = { "FULLY ASSOCIATIVE", "DIRECT MAPPED", "SET_ASSOCIATIVE" };
  static const char format_num[] = "\t%-20s %5lu\n";
  static const char format_bool[] = "\t%-20s %5s\n";

  printf("Printing Cache: %s\n", cache_name ? cache_name : "");
  printf("\t%-5s %20s\n", "mode", mode_str[cache->mode]);
  printf(format_num, "num_sets", cache->num_sets);
  printf(format_num, "set_size", cache->set_size);
  printf(format_num, "line_size", cache->line_size);
  printf(format_bool, "write", cache->write ? "true" : "false");
  
  printf("\t%-24s ", "offset_mask");
  printb(cache->offset_mask);
  printf("\n");

  if (cache->index_pos != ULONG_MAX)
    printf(format_num, "index_pos", cache->index_pos);
  else
    printf("\t%-10s %15s\n", "index_pos", "ULONG_MAX");

  printf("\t%-24s ", "index_mask");
  printb(cache->index_mask);
  printf("\n");

  printf(format_num, "addr_tag_pos", cache->addr_tag_pos);
  
  printf("\t%-24s ", "tag_mask");
  printb(cache->tag_mask);
  printf("\n");
  
  printf(format_num, "dirty_pos", cache->dirty_pos);
  printf(format_num, "valid_pos", cache->valid_pos);
  printf(format_num, "table_tag_pos", cache->table_tag_pos);
  printf(format_num, "table_num_frames", cache->table_num_frames);
  printf("\n");

  fflush(stdout);
}

void cache_connect(Cache* prev, Cache* next) {
  prev->next = next;
  next->prev = prev;
}

void _cache_decode(const Cache* cache, const uint32_t address, uint32_t* tag, uint32_t* index) {
  *index = (address >> cache->index_pos) & cache->index_mask;
  *tag = (address >> cache->addr_tag_pos) & cache->tag_mask;
}

// invalidates the entire cache. this isn't concerned with writing back dirty lines.
void cache_invalidate_all(Cache* cache) {
  memset(cache->table, 0, cache->table_num_frames * sizeof(CacheEntry));
}

void cache_free(Cache* cache) {
  free(cache->table);
  free(cache->stats);
  free(cache); 
}

// Assumes proper inputs
Cache* cache_new(const size_t num_sets, const size_t set_size, const size_t line_size, const bool write) {
  Cache* cache = malloc(sizeof(Cache));
  cache->stats = calloc(1, sizeof(CacheStats));
  
  // configuration information
  cache->num_sets = num_sets;
  cache->set_size = set_size;
  cache->line_size = line_size;
  cache->write = write;
  cache->next = cache->prev = NULL;

  if (num_sets == 1)
    cache->mode = FULLY_ASSOCIATIVE;
  else if (set_size == 1)
    cache->mode = DIRECT_MAPPED;
  else
    cache->mode = SET_ASSOCIATIVE;

  size_t bit_pos = 0;

  // addr offset decoding
  size_t num_offset_bits = log_2(line_size);
  cache->offset_mask = ~(ULONG_MAX << num_offset_bits);
  bit_pos += num_offset_bits;

  // addr index decoding
  size_t num_index_bits = log_2(num_sets);
  cache->index_pos = bit_pos;
  cache->index_mask = ~(ULONG_MAX << num_index_bits);
  bit_pos += num_index_bits;

  // addr tag decoding
  cache->addr_tag_pos = bit_pos;
  
  // shared
  size_t num_tag_bits = MAX_ADDR_LEN - bit_pos;
  cache->tag_mask = ~(ULONG_MAX << num_tag_bits);
  
  cache->valid_pos = cache->dirty_pos + 1;
  cache->table_tag_pos = cache->valid_pos + 1;
  
  // allcoate 'hardware' resources
  cache->table_num_frames = num_sets * set_size;
  cache->table = malloc(sizeof(CacheEntry) * cache->table_num_frames);
  if (!cache->table) {
    fprintf(stderr, "Failed to allocate entry table.\n");
    goto fail;
  }

  cache_invalidate_all(cache);

  return cache;

fail:
  free(cache);
  return NULL;
}

CacheStats* cache_stats(const Cache* cache) {
  return cache->stats;
}

// returns TRUE if tags in the entry and address match, returns FALSE otherwise
bool cache_tag_match(const Cache* cache, const uint32_t tag, const CacheEntry entry) {
  uint32_t entry_tag = (entry >> cache->table_tag_pos) & cache->tag_mask;
  return tag == entry_tag;
}

// returns TRUE on hit, FALSE on miss, only puts frame number into *frame on hit and frame is non-null.
// always puts set number in *set regardless of hit/miss if set is non-null.
bool _cache_check(const Cache* cache, const uint32_t tag, const uint32_t index, size_t* frame) {
  size_t start_frame = index * cache->set_size;
  for (size_t i = start_frame; i < start_frame + cache->set_size; i++) {
    CacheEntry entry = cache->table[i];
    
    bool valid = (entry >> cache->valid_pos) & 1;
    if (!valid) continue;
    else if (cache_tag_match(cache, tag, entry)) {
      if (frame) *frame = i;
      return true;
    }
  }
  
  return false;
}

CacheEntry _cache_new_entry(const Cache* cache, const uint32_t tag) {
  CacheEntry new_entry = tag << cache->table_tag_pos; 
  new_entry |= 1 << cache->valid_pos;
  return new_entry;
}

uint32_t _cache_address_from_tag_index(const Cache* cache, const uint32_t tag, const uint32_t index) {
  return ((tag & cache->tag_mask) << cache->addr_tag_pos) | ((index & cache->index_mask) << cache->index_pos);
}

void cache_writeback(Cache* cache, const uint32_t address) {
  (void) cache;
  (void) address;
  // write, but dont update the current address information
  // i dont know if this should also update the written to cache stats or not
  printf("called function cache_writeback\n");
}

void _cache_evict(const Cache* cache, const uint32_t index) {
  size_t start_frame = index * cache->set_size;
  size_t r = rand() % cache->set_size;

  // evict (WITHOUT WRITE BACK WRITE NOW)
  CacheEntry entry = cache->table[start_frame + r];
  bool dirty = (entry >> cache->dirty_pos) & 1;
  uint32_t dirty_tag = (entry >> cache->table_tag_pos) & cache->tag_mask;
  if (dirty) {
    if (cache->next)
      cache_writeback(cache->next, _cache_address_from_tag_index(cache, dirty_tag, index)); 
    else
    {
      //! TODO WRITE TO MEMORY
    }
  }

  cache->table[start_frame + r] = 0;
}

bool _cache_insert(const Cache* cache, const uint32_t tag, const uint32_t index) {
  size_t start_frame = index * cache->set_size;
  for (size_t i = start_frame; i < start_frame + cache->set_size; i++) {
    CacheEntry entry = cache->table[i];
    
    bool valid = (entry >> cache->valid_pos) & 1;
    if (valid) continue;
    
    cache->table[i] = _cache_new_entry(cache, tag);
    return true;
  }

  return false;
}

// invalidate a cache entry without writing back if dirty.
bool cache_invalidate_entry(Cache* cache, const uint32_t address) {
  size_t frame;
  
  uint32_t tag;
  uint32_t index;

  _cache_decode(cache, address, &tag, &index);

  if (!_cache_check(cache, tag, index, &frame)) return false;
  else cache->table[frame] = 0;

  return true;
}

bool cache_read(Cache* cache, const uint32_t address) {
  uint32_t tag;
  uint32_t index;
  _cache_decode(cache, address, &tag, &index);
  
  cache->stats->type = READ;
  cache->stats->address = address;
  cache->stats->tag = tag;
  cache->stats->index = index;

  cache->stats->total_accesses += 1;
  if (_cache_check(cache, tag, index, NULL)) {
    cache->stats->hits += 1;
    cache->stats->resolved = true;
    cache->stats->access_next = false;
    return true;
  }
  
  // If backing, read backing, otherwise go to memory
  if (cache->next) {
    cache->stats->access_next = true;
    cache_read(cache->next, address);
  }
  else {
    cache->stats->access_next = false;
    // goto memory
  }

  // this shouldnt have to be done more than once, than repeats can tell me if something is off
  while (!_cache_insert(cache, tag, index)) {
    _cache_evict(cache, index);
    // printf("evicting from index: %u\n", index);
  }

  cache->stats->resolved = false;
  return false;
}

void cache_write(Cache* cache, const uint32_t address) {
  uint32_t tag;
  uint32_t index;

  _cache_decode(cache, address, &tag, &index);
  if (_cache_check(cache, tag, index, NULL)) {             // HIT, set dirty bit and return 
    return;
  }
}
