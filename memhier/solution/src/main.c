#include <stdio.h>
#include <string.h>
#include <stdint.h>
#include <stdlib.h>
#include "config.h"
#include "cache.h"
#include "ptable.h"
#include "tlb.h"

#define LINESIZE 20

struct RefStats {
  size_t memory_refs;
  size_t pt_refs;
  size_t disk_refs;
};

typedef struct RefStats RefStats;

void print_cache_stats(const CacheStats* stats, const char* name) {
  printf("%2s %-14s: %lu\n", name, "hits", stats ? stats->hits : 0);
  printf("%2s %-14s: %lu\n", name, "misses", stats ? (stats->total_accesses - stats->hits) : 0);

  if (stats)
    printf("%2s %-14s: %lf\n", name, "hit ratio", (double) stats->hits / (double) stats->total_accesses);
  else
    printf("%2s %-14s: %s\n", name, "hit ratio", "N/A");
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

void print_ref_stats(const RefStats* ref_stats) {
  printf("%-17s: %lu\n", "main memory refs", ref_stats->memory_refs); 
  printf("%-17s: %lu\n", "page table refs", ref_stats->pt_refs);
  printf("%-17s: %lu\n", "disk refs", ref_stats->disk_refs);
}

void print_tlb_stats(const TLBStats* tlb_stats) {

  printf("%-17s: %lu\n", "dtlb hits", tlb_stats ? tlb_stats->hits : 0);
  printf("%-17s: %lu\n", "dtlb misses", tlb_stats ? (tlb_stats->total_accesses - tlb_stats->hits) : 0);
  
  if (tlb_stats)
    printf("%-17s: %lf\n", "dtlb hit ratio", (double) tlb_stats->hits / (double) tlb_stats->total_accesses);
  else
    printf("%-17s: %s\n", "dtlb hit ratio" , "N/A");
}


int main() {
  Config* config = read_config("../trace.config");
  if (!config) return 1;

  print_config(config);
  
  // PAGE TABLE
  PTable* ptable = ptable_new(config->pt_num_vpages, config->pt_num_ppages, config->pt_page_size);

  // TLB
  TLB* tlb = config->use_tlb ? TLB_new(ptable, config->tlb_num_sets, config->tlb_set_size, config->pt_page_size) : NULL;

  ptable_connect_tlb(ptable, tlb); 

  // DC CACHE
  Cache* dc = cache_new(config->dc_num_sets, config->dc_set_size, config->dc_line_size, config->dc_write ? WRITE_THROUGH : WRITE_BACK, config->dc_write ? NO_WRALLOC : WRALLOC);
  if (!dc) {
    fprintf(stderr, "Failed to initialize dc\n");
    return 1;
  }
  
  // L2 CACHE
  Cache* L2 = NULL;
  if (config->use_L2) {
    L2 = cache_new(config->L2_num_sets, config->L2_set_size, config->L2_line_size, config->L2_write ? WRITE_THROUGH : WRITE_BACK, config->L2_write ? NO_WRALLOC : WRALLOC);
    if (!L2) {
      fprintf(stderr, "Failed to initialize L2\n");
      return 1;
    }

    // CONNECT CACHES
    cache_connect(dc, L2);
    ptable_connect_cache(ptable, L2);
  } else {
    ptable_connect_cache(ptable, dc);
  }

  
  char* buf = malloc(LINESIZE);
  size_t size = LINESIZE;
  char read_write;
  uint32_t address; 
  uint32_t paddress;

  // STATS
  CacheStats* dc_stats = cache_stats(dc);
  CacheStats* L2_stats = config->use_L2 ? cache_stats(L2) : NULL;
  PTableStats* pt_stats = ptable_stats(ptable);
  TLBStats* tlb_stats = config->use_tlb ? TLB_stats(tlb) : NULL;

  strcpy(dc_stats->name, "dc");
  strcpy(L2_stats->name, "L2");

  printf("Virtual  Virt.  Page TLB    TLB TLB  PT   Phys        DC  DC          L2  L2\n");
  printf("Address  Page # Off  Tag    Ind Res. Res. Pg # DC Tag Ind Res. L2 Tag Ind Res.\n");
  printf("-------- ------ ---- ------ --- ---- ---- ---- ------ --- ---- ------ --- ----\n");

  while (getline(&buf, &size, stdin) != -1) {
    if (sscanf(buf, "%c:%x", &read_write, &address) != 2) {
      fprintf(stderr, "failed to parse\n");
      continue;
    }
    
    // ADDRESS TRANSLATION
    if (config->virtual_addresses) {
      // check if the virtual address is too large
      if (address > config->pt_page_size * config->pt_num_vpages) {
        fprintf(stderr, "virtual address too large\n");
        continue;
      }

      paddress = config->use_tlb ? TLB_virt_phys(tlb, address) : ptable_virt_phys(ptable, address); 
    }
    else {
      // check if the physical address is too large
      if (address > config->pt_page_size * config->pt_num_ppages) {
        fprintf(stderr, "physical address too large\n");
        continue;
      }
      paddress = address;
    }

    // reset inserts
    dc_stats->hit = false;
    if (config->use_L2)
      L2_stats->hit = false;
  
    // CACHE ACCESS
    switch (read_write) {
      case 'W':
        cache_write(dc, paddress, true);
        break;
      case 'R':
        cache_read(dc, paddress);
        break;
      default:
        printf("hierarchy: unexpected access type\n");
        goto cleanup;
    }

    // PRINT
    if (config->use_tlb) {
      printf("%08x %6x %4x %6x %3x %-4s %-4s %4x %6x %3x %-5s", address, tlb_stats->vpage, tlb_stats->offset, tlb_stats->tag, tlb_stats->index, tlb_stats->hit ? "hit": "miss", tlb_stats->hit ? "    " : (pt_stats->hit ? "hit" : "miss"), tlb_stats->ppage, dc_stats->tag, dc_stats->index, dc_stats->hit ? "hit" : "miss");
    } else {
      printf("%08x %6x %4x                 %-4s %4x %6x %3x %-5s", address, pt_stats->vpage, pt_stats->offset, pt_stats->hit ? "hit" : "miss", pt_stats->ppage, dc_stats->tag, dc_stats->index, dc_stats->hit ? "hit" : "miss");
    }

    if (config->use_L2 && (L2_stats->hit || !dc_stats->hit))
      printf("%6x %3x %-4s\n", L2_stats->tag, L2_stats->index, L2_stats->hit ? "hit" : "miss");
    else
      printf("\n");
  }

  RefStats ref_stats;
  ref_stats.memory_refs = config->use_L2 ? L2_stats->mem_accesses : dc_stats->mem_accesses;
  ref_stats.pt_refs = pt_stats->total_accesses;
  ref_stats.disk_refs = pt_stats->disk_accesses;
  
  printf("\nSimulation statistics\n\n");

  // PRINT EVERYTHING
  print_tlb_stats(tlb_stats);
  fputc('\n', stdout);
  print_pt_stats(pt_stats);
  fputc('\n', stdout);
  print_cache_stats(dc_stats, "dc");
  fputc('\n', stdout);
  print_cache_stats(L2_stats, "L2");
  fputc('\n', stdout);
  print_rw_stats(dc_stats->reads, dc_stats->total_accesses - dc_stats->reads);
  fputc('\n', stdout);
  print_ref_stats(&ref_stats);


  // REQUIREMENTS
  // Max Reference address length is 32 bits
  // Inclusive Multi-level Caching Policy
  // LRU replacement for TLB, DC, L2, and Page Table
  // PAGE FAULT: Invalidate associated TLB, DC, and L2 entries 
cleanup:
  free_config(config);
  cache_free(dc);
  if (L2) cache_free(L2);
}
