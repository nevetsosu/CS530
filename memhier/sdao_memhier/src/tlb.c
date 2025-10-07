#include <stddef.h>
#include <stdlib.h>
#include <stdio.h>

typedef struct TLBEntry TLBEntry;
typedef struct DecodeConstants DecodeConstants;

#include "set.h"
#include "tlb.h"
#include "util.h"

struct DecodeConstants {
  size_t index_pos;
  size_t tag_pos;

  uint32_t tag_mask;
  uint32_t index_mask;
  uint32_t offset_mask;
};

struct TLBEntry {
  size_t page;
  uint32_t tag;
  bool   valid;
};

struct TLB {
  size_t num_sets;
  size_t set_size;
  size_t page_size;

  DecodeConstants decode;

  Set** sets;
  TLBStats* stats;

  PTable* ptable;
};

void _TLB_calculate_decode(TLB* tlb) {
  DecodeConstants* decode = &tlb->decode;

  decode->index_pos = log_2(tlb->page_size);
  decode->tag_pos = decode->index_pos + log_2(tlb->num_sets);

  decode->tag_mask = ~((~0u) << decode->tag_pos);
  decode->index_mask = ~((~0u) << (decode->tag_pos - decode->index_pos));
  decode->offset_mask = ~((~0u) << decode->index_pos);

}

TLB* TLB_new(PTable* ptable, const size_t num_sets, const size_t set_size, const size_t page_size) {
  TLB* tlb = malloc(sizeof(TLB));
  tlb->sets = malloc(sizeof(Set*) * num_sets);
  
  // make sets and TLBentries, connect them together
  for (size_t i = 0; i < num_sets; i++) {
    Set* set = tlb->sets[i] = Set_new(set_size);
    
    TLBEntry* entries = calloc(set_size + 1, sizeof(TLBEntry));

    for (size_t i = 0; i < set_size + 1; i++) {
      set->node_list[i].data = entries + i;
    }
  }

  tlb->stats = calloc(1, sizeof(TLBStats));
  
  // assign other member variables
  tlb->num_sets = num_sets;
  tlb->set_size = set_size;
  tlb->page_size = page_size;
  tlb->ptable = ptable;

  _TLB_calculate_decode(tlb);

  return tlb;
}
void TLB_free(TLB* tlb) {
  for(size_t i = 0; i < tlb->num_sets; i++) {
    Set* set = tlb->sets[i];
    free(set->node_list->data);
    Set_free(set);
  }

  free(tlb->sets);

  free(tlb->stats);
  free(tlb);
}

TLBStats* TLB_stats(const TLB* tlb) {
  return tlb->stats;
}

void _TLB_decode(TLB* tlb, uint32_t v_addr, uint32_t* tag, uint32_t* index) {
  *tag = (v_addr >> tlb->decode.tag_pos) & tlb->decode.tag_mask;
  *index = (v_addr >> tlb->decode.index_pos) & tlb->decode.index_mask; 
}

bool _TLB_get(TLB* tlb, const uint32_t tag, const uint32_t index, uint32_t* ppage, bool write) {
  Set* set = tlb->sets[index];
  TLBEntry* entry;

  // attempt to find
  SetNode* node;
  SET_TRAVERSE_RIGHT(node, set->node_list) {
    entry = (TLBEntry*) node->data;
    if (entry->valid && entry->tag == tag) {

      *ppage = entry->page;
      Set_set_mru(set, node);
      return true;
    
    }
  }

  // find invalid or LRU
  SET_TRAVERSE_LEFT(node, set->node_list) {
    entry = (TLBEntry*) node->data;
    if (entry->valid) continue;
    else goto L_tlb_assign;
  }
  node = Set_get_lru(set);

L_tlb_assign:
  // handoff translation to ptable
  entry = (TLBEntry*) node->data;

  uint32_t reconstructed_v_addr = (tag & tlb->decode.tag_mask) << tlb->decode.tag_pos;
  reconstructed_v_addr |= (index & tlb->decode.index_mask) << tlb->decode.index_pos;
  *ppage = ptable_virt_phys(tlb->ptable, reconstructed_v_addr, write) >> tlb->decode.index_pos;

  // cache entry
  entry->valid = true;
  entry->tag = tag;
  entry->page = *ppage;

  // set mru
  Set_set_mru(set, node);
  return false;
}

uint32_t TLB_virt_phys(TLB* tlb, const uint32_t v_addr, bool write) {
  tlb->stats->total_accesses += 1;
  tlb->stats->offset = v_addr & tlb->decode.offset_mask;
  tlb->stats->vpage = (v_addr >> tlb->decode.index_pos) & tlb->decode.tag_mask;

  _TLB_decode(tlb, v_addr, &tlb->stats->tag, &tlb->stats->index);
  tlb->stats->hit = _TLB_get(tlb, tlb->stats->tag, tlb->stats->index, &tlb->stats->ppage, write);
  if (tlb->stats->hit)
    tlb->stats->hits += 1;
  
  uint32_t p_addr = (tlb->stats->ppage << tlb->decode.index_pos) | tlb->stats->offset;
  return p_addr; 
}

// Traverse the entire TLB and invalidate any page mapping to 'ppage'
void TLB_invalidate_ppage(TLB* tlb, const uint32_t ppage) {
  for (size_t i = 0; i < tlb->num_sets; i++) {
    Set* set = tlb->sets[i];

    SetNode* node;
    SET_TRAVERSE_RIGHT(node, set->node_list) {
      TLBEntry* entry = (TLBEntry*) node->data;
      if (!entry->valid || entry->page != ppage) continue;
      
      entry->valid = false;
    }
  }
}
