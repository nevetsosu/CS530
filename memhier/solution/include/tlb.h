#pragma once
#include <stddef.h>
#include <stdbool.h>
#include <stdint.h>
#include "ptable.h"

typedef struct TLBStats TLBStats;
typedef struct TLB TLB;

struct TLBStats {
  uint32_t address;
  uint32_t tag;
  uint32_t index;
  uint32_t offset;
  uint32_t ppage;
  uint32_t vpage;

  bool hit;

  size_t hits;
  size_t total_accesses;
};

TLB* TLB_new(PTable* ptable, const size_t num_sets, const size_t set_size, const size_t page_size);
void TLB_free(TLB* tlb);
TLBStats* TLB_stats(const TLB* tlb);
uint32_t TLB_virt_phys(TLB* tlb, const uint32_t v_addr);


