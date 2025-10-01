#pragma once
#include <stddef.h>
#include <stdbool.h>

typedef struct Config {
  size_t tlb_num_sets;      // TLB Number of sets
  size_t tlb_set_size;      // TLB Set size

  size_t pt_num_vpages;     // PAGE TABLE Number of virtual pages
  size_t pt_num_ppages;     // PAGE TABLE Number of physical pages
  size_t pt_page_size;      // PAGE TABLE Page size

  size_t dc_num_sets;       // DIRECT CACHE Number of sets
  size_t dc_set_size;       // DIRECT CACHE Set size
  size_t dc_line_size;      // DIRECT CACHE Line size
  bool   dc_write;          // TRUE: Write Through, FALSE: No Write Allocate
  
  size_t L2_num_sets;       // L2 CACHE Number of sets
  size_t L2_set_size;       // L2 CACHE Set Size
  size_t L2_line_size;      // L2 CACHE Line Size
  bool   L2_write;          // TRUE: Write Through, FALSE: No Write Allocate
  
  bool virtual_addresses;   // TRUE: Require Virtual to Physical Translation
  bool use_tlb;             // TRUE: Use TLB, FALSE: Don't use TLB
  bool use_L2;              // TRUE: Use L2 Cache, FALSE: Don't use L2 Cache
} Config;

void print_config(const Config* config);
void free_config(Config* config);
Config* read_config(const char* filename);
