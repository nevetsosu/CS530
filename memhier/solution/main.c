#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include "config.h"
#include "cache.h"

#define LINESIZE 20

int main() {
  Config* config = read_config("../trace.config");
  if (!config) return 1;

  // DEBUG
  print_config(config);
  
  Cache* dc = cache_new(config->dc_num_sets, config->dc_set_size, config->dc_line_size, config->dc_write);
  if (!dc) {
    fprintf(stderr, "Failed to initialize dc\n");
    return 1;
  }
  cache_decode_debug(dc, "DC");

  Cache* L2 = cache_new(config->L2_num_sets, config->L2_set_size, config->L2_line_size, config->L2_write);
  if (!L2) {
    fprintf(stderr, "Failed to initialize L2\n");
    return 1;
  }
  cache_decode_debug(L2, "L2");

  // Connect the two caches
  cache_connect(dc, L2);
  
  char* buf = malloc(LINESIZE);
  size_t size = LINESIZE;
  char read_write;
  uint32_t address; 

  CacheStats* dc_stats = cache_stats(dc);
  CacheStats* L2_stats = cache_stats(L2);

  while (getline(&buf, &size, stdin) != -1) {
    sscanf(buf, "%c:%x", &read_write, &address);

    bool hit;
    switch (read_write) {
      case 'W':
        break;
      case 'R':
        hit = cache_read(dc, address);
        break;
      default:
        printf("hierarchy: unexpected access type\n");
        goto cleanup;
    }
    
    printf("%4x, %s, index: %u, tag: %u, resolved: %d\n", dc_stats->address, dc_stats->type == READ ? "read" : "write", dc_stats->index, dc_stats->tag, dc_stats->resolved);
  }

  // REQUIREMENTS
  // Max Reference address length is 32 bits
  // Inclusive Multi-level Caching Policy
  // LRU replacement for TLB, DC, L2, and Page Table
  // PAGE FAULT: Invalidate associated TLB, DC, and L2 entries 
cleanup:
  free_config(config);
  cache_free(dc);
  cache_free(L2);
}
