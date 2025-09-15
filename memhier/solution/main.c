#include <stdio.h>
#include "config.h"
#include "cache.h"

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

  // REQUIREMENTS
  // Max Reference address length is 32 bits
  // Inclusive Multi-level Caching Policy
  // LRU replacement for TLB, DC, L2, and Page Table
  // PAGE FAULT: Invalidate associated TLB, DC, and L2 entries
  
  free_config(config);
}
