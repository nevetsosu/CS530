#include <stddef.h>
#include <stdbool.h>
#include <stdlib.h>

#include "util.h"
#include "ptable.h"
#include "palloc.h"

typedef struct PTableEntry PTableEntry;
typedef struct PTable PTable;
typedef struct PTableStats PTableStats;

struct PTableEntry {
  size_t page;
  bool valid;
};

struct PTable {
  size_t vpages;
  size_t ppages;
  size_t page_size;
  
  size_t offset_bits;
  uint32_t page_offset_mask;

  PAlloc* palloc;
  PTableStats* stats;
  PTableEntry* table; // replace this with a Set, Use the sentinel to access the nodelist for O(1) random access by index.
};

PTable* ptable_new(const size_t virtual_pages, const size_t ppages, const size_t page_size) { 
  PTable* ptable = malloc(sizeof(PTable));
  ptable->vpages = virtual_pages;
  ptable->ppages = ppages;
  ptable->page_size = page_size;
  ptable->offset_bits = log_2(page_size);
  ptable->page_offset_mask = ~(~0u << ptable->offset_bits);
  ptable->table = calloc(virtual_pages, sizeof(PTableEntry));
  ptable->palloc = palloc_new(ppages);
  ptable->stats = malloc(sizeof(PTableStats));
  return ptable;
}

void ptable_free(PTable* ptable) {
  free(ptable->table);
  free(ptable->stats);
  palloc_free(ptable->palloc);
  free(ptable);
}

bool _ptable_get(PTable* ptable, uint32_t vpage, uint32_t* ppage) {
  if (ptable->table[vpage].valid) {
    *ppage = ptable->table[vpage].page;
    return true;
  }

  // missed, allocate new page
  *ppage = ptable->table[vpage].page = palloc_new_page(ptable->palloc);
  ptable->table[vpage].valid = true;
  
  return false;
}

PTableStats* ptable_stats(const PTable* ptable) {
  return ptable->stats;
}

uint32_t ptable_virt_phys(PTable* ptable, const uint32_t address) {
  ptable->stats->total_accesses += 1;
  ptable->stats->offset = address & ptable->page_offset_mask;

  ptable->stats->vpage = address >> ptable->offset_bits;
  if ((ptable->stats->hit = _ptable_get(ptable, ptable->stats->vpage, &ptable->stats->ppage)))
    ptable->stats->hits += 1;
  
  return (ptable->stats->ppage << ptable->offset_bits) | ptable->stats->offset;  
}
