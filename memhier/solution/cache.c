#include <stdint.h>
#include <stddef.h>
#include <stdbool.h>
#include <stdlib.h>
#include <limits.h>
#include <stdio.h>
#include "config_consts.h"
#include "util.h"

typedef uint64_t CacheEntry;
typedef int CacheMode;

#define FULLY_ASSOCIATIVE 0
#define DIRECT_MAPPED 1
#define SET_ASSOCIATIVE 2

#define CACHE_ENTRY_BITSIZE (sizeof(CacheEntry) * 8)

typedef struct Cache {
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
  size_t LRU_num_bits;
  size_t dirty_pos;
  size_t valid_pos;
  size_t table_tag_pos;

  // shared addr and table
  size_t tag_mask;
  
  // 'hardware' storage
  size_t table_num_frames;
  CacheEntry* table;

} Cache;

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
  
  printf(format_num, "LRU_num_bits", cache->LRU_num_bits);
  printf(format_num, "dirty_pos", cache->dirty_pos);
  printf(format_num, "valid_pos", cache->valid_pos);
  printf(format_num, "table_tag_pos", cache->table_tag_pos);
  printf(format_num, "table_num_frames", cache->table_num_frames);
  printf("\n");

  fflush(stdout);
}

void cache_invalidate(Cache* cache) {
  for (size_t i = 0; i < cache->table_num_frames; i++) {
    cache->table[i] &= ~(1lu << cache->valid_pos);
  }
}

void cache_free(Cache* cache) {
  free(cache->table);
  free(cache); 
}

// Assumes proper inputs
Cache* cache_new(const size_t num_sets, const size_t set_size, const size_t line_size, const bool write) {
  Cache* cache = malloc(sizeof(Cache));
  
  // configuration information
  cache->num_sets = num_sets;
  cache->set_size = set_size;
  cache->line_size = line_size;
  cache->write = write;

  // determine operation mode
  if (set_size == 1)
    cache->mode = DIRECT_MAPPED;
  else if (num_sets == 1) 
    cache->mode = FULLY_ASSOCIATIVE;
  else
    cache->mode = SET_ASSOCIATIVE;
  
  size_t bit_pos = 0;

  // addr offset decoding
  size_t num_offset_bits = log_2(line_size);
  cache->offset_mask = ~(ULONG_MAX << num_offset_bits);
  bit_pos += num_offset_bits;

  // addr index decoding
  if (cache->mode != FULLY_ASSOCIATIVE) {
    size_t num_index_bits = log_2(num_sets);
    cache->index_pos = bit_pos;
    cache->index_mask = ~(ULONG_MAX << num_index_bits);
    bit_pos += num_index_bits;
  } else {
    cache->index_pos = ULONG_MAX;
    cache->index_mask = ULONG_MAX;
  }

  // addr tag decoding
  cache->addr_tag_pos = bit_pos;
  
  // shared
  size_t num_tag_bits = MAX_ADDR_LEN - bit_pos;
  cache->tag_mask = ~(ULONG_MAX << num_tag_bits);
  
  // // table entry LRU bits
  // cache->LRU_num_bits = cache->dirty_pos = (cache->mode == DIRECT_MAPPED) ? 0 : set_size;
  
  // TEMPORARY NO LRU BITS
  cache->LRU_num_bits = cache->dirty_pos = 0;

  cache->valid_pos = cache->dirty_pos + 1;
  cache->table_tag_pos = cache->valid_pos + 1;
  
  // allcoate 'hardware' resources
  cache->table_num_frames = num_sets * set_size;
  cache->table = malloc(sizeof(CacheEntry) * cache->table_num_frames);
  if (!cache->table) {
    fprintf(stderr, "Failed to allocate entry table.\n");
    goto fail;
  }

  cache_invalidate(cache);

  return cache;

fail:
  free(cache);
  return NULL;
}

// returns TRUE on hit, FALSE on miss, only puts frame number into *frame on hit and frame is non-null.
bool cache_check_fully(const Cache* cache, uint32_t address, size_t* frame) {
  if (cache->mode != FULLY_ASSOCIATIVE) {
    fprintf(stderr, "WARNING A non-FULLY-ASSOCIATIVE cache has been passed to the FULLY ASSOCIATIVE check.\n");
    return false;
  }

  for (size_t set = 0; set < cache->table_num_frames; set++) {
    CacheEntry entry = cache->table[set];
    uint32_t addr_tag = (address >> cache->addr_tag_pos) & cache->tag_mask;
    uint32_t entry_tag = (entry >> cache->table_tag_pos) & cache->tag_mask;
    bool valid = (entry >> cache->valid_pos) & 1;

    if (addr_tag == entry_tag && valid) {
      if (frame) *frame = set;
      return true;
    }
  }

  return false;
}

// returns TRUE on hit, FALSE on miss, will always put frame number in *frame if non-null.
bool cache_check_direct(const Cache* cache, uint32_t address, size_t* frame) {
  if (cache->mode != DIRECT_MAPPED) {
    fprintf(stderr, "WARNING A non-DIRECT-MAPPED cache has been passed to the DIRECT MAPPED check.\n");
    return false;
  }

  size_t set = (address << cache->index_pos) & cache->index_mask;
  if (frame) *frame = set;

  CacheEntry entry = cache->table[set];
  bool valid = (entry >> cache->valid_pos) & 1;
  if (!valid) return false;

  uint32_t addr_tag = (address << cache->addr_tag_pos) & cache->tag_mask;
  uint32_t table_tag = (address << cache->table_tag_pos) & cache->tag_mask;
 
  return addr_tag == table_tag; 
}

// returns TRUE on hit, FALSE on miss, only puts frame number into *frame on hit and frame is non-null.
bool cache_check_set(const Cache* cache, uint32_t address, size_t* frame) {
  if (cache->mode != SET_ASSOCIATIVE) {
    fprintf(stderr, "WARNING A non-SET-ASSOCIATIVE cache has been passed to the SET_ASSOCIATIVE check.\n");
    return false;
  }
  
  size_t set = (address >> cache->index_pos) & cache->index_mask;
  size_t start_frame = set * cache->set_size;
  for (size_t i = start_frame; i < start_frame + set_size; i++) {
    CacheEntry entry = cache->table[i];
    
    bool valid = (entry >> cache->valid_pos & 1);
    if (!valid) continue;

    uint32_t addr_tag = (address << cache->addr_tag_pos) & cache->tag_mask;
    uint32_t table_tag = (address << cache->table_tag_pos) & cache->tag_mask;
    
    if (addr_tag == table_tag) {
      if (frame) *frame = i;
      return true;
    }
  }
  
  return false;
}

bool cache_check(const Cache* cache, uint32_t address) {
  switch(cache->mode) {
    case FULLY_ASSOCIATIVE:
      return cache_check_fully(cache, address);
    case DIRECT_MAPPED:
      return cache_check_direct(cache, address);
    case SET_ASSOCIATIVE:
      return cache_check_set(cache, address);
  }
}
