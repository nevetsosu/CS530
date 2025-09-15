#include <stdint.h>
#include <stddef.h>
#include <stdbool.h>
#include <stdlib.h>
#include <limits.h>
#include <stdio.h>
#include "config_consts.h"
#include "util.h"

typedef uint32_t CacheEntry;
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
  size_t table_size;
  size_t store_size;
  CacheEntry* table;
  uint8_t* store;

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

  printf(format_num, "index_pos", cache->index_pos);

  printf("\t%-24s ", "index_mask");
  printb(cache->index_mask);
  printf("\n");

  printf(format_num, "addr_tag_pos", cache->addr_tag_pos);
  
  printf("\t%-24s ", "tag_mask");
  printb(cache->tag_mask);
  printf("\n");
  
  printf(format_num, "LRU_num_bit", cache->LRU_num_bits);
  printf(format_num, "dirty_pos", cache->dirty_pos);
  printf(format_num, "valid_pos", cache->valid_pos);
  printf(format_num, "table_tag_pos", cache->table_tag_pos);
  printf(format_num, "table_size", cache->table_size);
  printf(format_num, "store_size", cache->store_size);
  printf("\n");

  fflush(stdout);
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
  
  // table entry LRU bits
  cache->LRU_num_bits = cache->dirty_pos = (cache->mode == DIRECT_MAPPED) ? 0 : set_size; 
  cache->valid_pos = cache->dirty_pos + 1;
  cache->table_tag_pos = cache->valid_pos + 1;
  
  // allcoate 'hardware' resources
  cache->table_size = sizeof(CacheEntry) * num_sets * set_size;
  cache->store_size = num_sets * set_size * line_size;
  cache->table = malloc(cache->table_size);
  cache->store = malloc(cache->store_size);
  if (!cache->table || !cache->store) {
    fprintf(stderr, "Failed to allocate cache resources.\n");
    goto fail;
  }

  return cache;

fail:
  free(cache);
  return NULL;
}


