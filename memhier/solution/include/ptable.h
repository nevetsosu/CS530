#include <stddef.h>
#include <stdbool.h>
#include <stdint.h>

typedef struct PTableEntry PTableEntry;
typedef struct PTable PTable;
typedef struct PTableStats PTableStats;

struct PTableStats {
  uint32_t vpage;
  uint32_t ppage;
  uint32_t offset;
  bool hit;

  size_t hits;
  size_t total_accesses;
};

PTable* ptable_new(size_t virtual_pages, size_t physical_pages, size_t page_size);
void ptable_free(PTable* ptable);
PTableStats* ptable_stats(const PTable* ptable);
uint32_t ptable_virt_phys(PTable* ptable, const uint32_t address);
