#include <stddef.h>
#include <stdbool.h>
#include <stdlib.h>
#include <stdio.h>

#include "util.h"
#include "ptable.h"
#include "set.h"
#include "tlb.h"
#include "cache.h"

typedef struct TableEntry TableEntry;
typedef struct PTable PTable;
typedef struct PTableStats PTableStats;

struct TableEntry {
  size_t page;
  bool dirty;
  bool valid;
};

struct PTable {
  size_t vpages;
  size_t ppages;
  size_t page_size;
  
  size_t offset_bits;
  uint32_t page_offset_mask;

  PTableStats* stats;

  // Page Table
  TableEntry* vpage_table;

  // Inverse Table
  Set* ppage_set;
  TableEntry* ppage_table;
  size_t cur_ppage;

  TLB* tlb;
  Cache* cache;
};

PTable* ptable_new(const size_t vpages, const size_t ppages, const size_t page_size) { 
  PTable* ptable = malloc(sizeof(PTable));
  ptable->vpages = vpages;
  ptable->ppages = ppages;
  ptable->page_size = page_size;
  ptable->offset_bits = log_2(page_size);
  ptable->page_offset_mask = ~(~0u << ptable->offset_bits);
  ptable->stats = malloc(sizeof(PTableStats));
  ptable->ppage_set = Set_new(ppages);
  ptable->ppage_table = calloc(ppages + 1, sizeof(TableEntry));
  ptable->vpage_table = calloc(vpages, sizeof(TableEntry));
  
  // Connect the ptable_set and ptable_table
  SetNode* node_list = ptable->ppage_set->node_list;
  for (size_t i = 0; i < ppages + 1; i++) {
    node_list[i].data = ptable->ppage_table + i;
  }
  
  // ppage_table should start one after sentinel
  ptable->ppage_table += 1;

  return ptable;
}

// connects tlb for invalidation when a page is evicted
void ptable_connect_tlb(PTable* ptable, TLB* tlb) {
  ptable->tlb = tlb;
}

// connects cache for invalidation when a page is evicted
void ptable_connect_cache(PTable* ptable, Cache* cache) {
  ptable->cache = cache;
}

void ptable_free(PTable* ptable) {
  free(ptable->stats); 
  free(ptable->vpage_table);
  free(ptable->ppage_table - 1);
  Set_free(ptable->ppage_set);
  free(ptable);
}

void _ptable_update(PTable* ptable, uint32_t vpage, uint32_t ppage) {
  TableEntry* p_entry = ptable->ppage_table + ptable->cur_ppage;
  TableEntry* v_entry = ptable->vpage_table + vpage;

  v_entry->valid = p_entry->valid = true;

  v_entry->page = ppage;
  p_entry->page = vpage;
  
  Set_set_mru(ptable->ppage_set, ptable->ppage_set->node_list + 1 + ppage);
}

uint32_t _ptable_evict(PTable* ptable) {
  SetNode* lru = Set_get_lru(ptable->ppage_set);
  TableEntry* p_entry = (TableEntry*) lru->data;
  uint32_t ppage = p_entry - ptable->ppage_table;
  
  fprintf(stderr, "[PTABLE] Evicting page: %u\n", ppage);
  if (!p_entry->valid)
    fprintf(stderr, "The evictor was called but LRU was already invalid?");
  
  if (p_entry->dirty) {
    ptable->stats->disk_accesses += 1;
    fprintf(stderr, "[PTABLE] evicting dirty page\n");
  }
  ptable->vpage_table[p_entry->page].valid = false;

  return ppage;
} 

bool _ptable_get(PTable* ptable, uint32_t vpage, uint32_t* ppage, bool write) {
  TableEntry* v_entry = ptable->vpage_table + vpage;
  bool hit = false;

    // hit, return entry
  if (v_entry->valid) {
    hit = true;
    *ppage = v_entry->page;  
    goto L_update_ptable;
  }
  
  // disk_access for page read
  ptable->stats->disk_accesses += 1;

  // search for free physical page
  for (size_t i = 0; i < ptable->ppages; ptable->cur_ppage = (ptable->cur_ppage + 1) % ptable->ppages, i++) {
    TableEntry* p_entry = ptable->ppage_table + ptable->cur_ppage;
    if (p_entry->valid) continue;
    
    *ppage = ptable->cur_ppage; 
    goto L_update_ptable;
  }

  // evict and reassign a page
  *ppage = ptable->cur_ppage = _ptable_evict(ptable);
  if (ptable->tlb) TLB_invalidate_ppage(ptable->tlb, *ppage);

  uint32_t low_addr = *ppage * ptable->page_size;
  if (ptable->cache) cache_invalidate_range(ptable->cache, low_addr, low_addr + ptable->page_size - 1);

L_update_ptable:

  if (write) {
    fprintf(stderr, "[PTABLE] writing page\n");
    ptable->ppage_table[*ppage].dirty = true;
  }

  _ptable_update(ptable, vpage, *ppage); 
  Set_set_mru(ptable->ppage_set, ptable->ppage_set->node_list + 1 + *ppage);

  fprintf(stderr, "[PTABLE] setting to new mru: %u\n", *ppage);
  return hit;
}

PTableStats* ptable_stats(const PTable* ptable) {
  return ptable->stats;
}

uint32_t ptable_virt_phys(PTable* ptable, const uint32_t v_addr, bool write) {
  ptable->stats->total_accesses += 1;
  ptable->stats->offset = v_addr & ptable->page_offset_mask;

  ptable->stats->vpage = v_addr >> ptable->offset_bits;
  ptable->stats->hit = _ptable_get(ptable, ptable->stats->vpage, &ptable->stats->ppage, write);
  if (ptable->stats->hit)
    ptable->stats->hits += 1;
  
  return (ptable->stats->ppage << ptable->offset_bits) | ptable->stats->offset;  
}
