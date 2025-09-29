#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include "config.h"
#include "cache.h"
#include "ptable.h"

#define LINESIZE 20

void print_cache_stats(const CacheStats* stats, const char* name) {
  size_t misses = stats->total_accesses - stats->hits;
  printf("%2s %-14s: %lu\n", name, "hits", stats->hits);
  printf("%2s %-14s: %lu\n", name, "faults", misses);
  printf("%2s %-14s: %lf\n", name, "pt hit ratio", (double) stats->hits / (double) stats->total_accesses);
}

void print_rw_stats(const size_t reads, const size_t writes) {
  printf("%-17s: %lu\n", "Total reads", reads);
  printf("%-17s: %lu\n", "Total writes", writes);
  printf("%-17s: %lf\n", "Ratio of reads", (double) reads / (double)(reads + writes));
}

void print_pt_stats(const PTableStats* ptable) {
  printf("%-17s: %lu\n", "pt hits", ptable->hits);
  printf("%-17s: %lu\n", "pt faults", ptable->total_accesses - ptable->hits);
  printf("%-17s: %lf\n", "pt hit ratio", (double) ptable->hits / (double) ptable->total_accesses); 
}

int main() {
  Config* config = read_config("../trace.config");
  if (!config) return 1;

  // DEBUG
  print_config(config);
  
  // PAGE TABLE
  PTable* ptable = ptable_new(config->pt_num_vpages, config->pt_num_ppages, config->pt_page_size);

  // DC CACHE
  Cache* dc = cache_new(config->dc_num_sets, config->dc_set_size, config->dc_line_size, config->dc_write);
  if (!dc) {
    fprintf(stderr, "Failed to initialize dc\n");
    return 1;
  }
  cache_decode_debug(dc, "DC");
  
  // L2 CACHE
  Cache* L2 = cache_new(config->L2_num_sets, config->L2_set_size, config->L2_line_size, config->L2_write);
  if (!L2) {
    fprintf(stderr, "Failed to initialize L2\n");
    return 1;
  }
  cache_decode_debug(L2, "L2");

  // CONNECT CACHES
  cache_connect(dc, L2);
  
  char* buf = malloc(LINESIZE);
  size_t size = LINESIZE;
  char read_write;
  uint32_t address; 
  
  size_t reads = 0;
  size_t writes = 0;
  
  // STATS
  CacheStats* dc_stats = cache_stats(dc);
  CacheStats* L2_stats = cache_stats(L2);
  PTableStats* pt_stats = ptable_stats(ptable);

  printf("Virtual  Virt.  Page TLB    TLB TLB  PT   Phys        DC  DC          L2  L2\n");
  printf("Address  Page # Off  Tag    Ind Res. Res. Pg # DC Tag Ind Res. L2 Tag Ind Res.\n");
  printf("-------- ------ ---- ------ --- ---- ---- ---- ------ --- ---- ------ --- ----\n");

  while (getline(&buf, &size, stdin) != -1) {
    sscanf(buf, "%c:%x", &read_write, &address);
    
    // ADDRESS TRANSLATION
    if (config->virtual_addresses) {
      // check if the virtual address is too large

      if (config->use_tlb) {
        printf("TLB isn't ready yet");
        address = ptable_virt_phys(ptable, address);
        //! TODO address = tlb_translation
      } else
        address = ptable_virt_phys(ptable, address); 
    } else {
      // check if the physical address is too large
    }

    switch (read_write) {
      case 'W':
        writes += 1;
        break;
      case 'R':
        reads += 1;
        cache_read(dc, address);
        break;
      default:
        printf("hierarchy: unexpected access type\n");
        goto cleanup;
    }
    printf("%08x %6x %4x %6x %3x %-4s %-4s %4x %6x %3x %-4s", address, pt_stats->vpage, pt_stats->offset, 0, 0, "N/A", pt_stats->hit ? "hit" : "miss", pt_stats->ppage, dc_stats->tag, dc_stats->index, dc_stats->resolved ? "hit" : "miss");
    if (dc_stats->access_next)
      printf(" %6x %3x %4s\n", L2_stats->tag, L2_stats->index, L2_stats->resolved ? "hit" : "miss");
    else
      fputc('\n', stdout);
  }
  
  printf("\nSimiulation Statistics\n\n");

  // PRINT EVERYTHING
  print_pt_stats(pt_stats);
  fputc('\n', stdout);
  print_cache_stats(dc_stats, "dc");
  fputc('\n', stdout);
  print_cache_stats(L2_stats, "L2");
  fputc('\n', stdout);
  print_rw_stats(reads, writes);

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
