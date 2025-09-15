#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "config.h"
#include "config_consts.h"

void print_config(const Config* config) {
  printf("Data TLB configuration\n");
  printf("\tNumber of sets: %lu\n", config->tlb_num_sets);
  printf("\tSet size: %lu\n\n", config->tlb_set_size);
  
  printf("Page Table configuration\n");
  printf("\tNumber of virtual pages: %lu\n", config->pt_num_vpages);
  printf("\tNumber of physical pages: %lu\n", config->pt_num_ppages);
  printf("\tPage size: %lu\n\n", config->pt_page_size);

  printf("Data Cache configuration\n");
  printf("\tNumber of sets: %lu\n", config->dc_num_sets);
  printf("\tSet size: %lu\n", config->dc_set_size);
  printf("\tLine size: %lu\n", config->dc_line_size);
  printf("\tWrite through/no write allocate: %c\n\n", config->dc_write ? 'y' : 'n');
  
  printf("L2 Cache configuration\n");
  printf("\tNumber of sets: %lu\n", config->L2_num_sets);
  printf("\tSet size: %lu\n", config->L2_set_size);
  printf("\tLine size: %lu\n", config->L2_line_size);
  printf("\tWrite through/no write allocate: %c\n\n", config->L2_write ? 'y' : 'n');

  printf("Toggles\n");
  printf("\tVirtual addresses: %c\n", config->virtual_addresses ? 'y' : 'n');
  printf("\tTLB: %c\n", config->use_tlb ? 'y' : 'n');
  printf("\tL2: %c\n\n", config->use_L2 ? 'y' : 'n');
}

void free_config(Config* config) {
  free(config);
}

bool is_power2(size_t n) {
  return n != 0 && (n & (n - 1)) == 0;
}

bool validate_config(const Config* config) {
  // MAX TLB sets
  if (config->tlb_num_sets > TLB_MAX_SETS) {
    fprintf(stderr, "TLB Max number of sets is %lu.\n", TLB_MAX_SETS);
    return false;
  }

  // MAX DC sets
  if (config->dc_num_sets > DC_MAX_SETS) {
    fprintf(stderr, "DC Max number of sets is %lu.\n", DC_MAX_SETS);
    return false;
  }

  // Max set size (associativity)
  if (config->tlb_set_size > MAX_ASSOCIATIVITY || \
      config->dc_set_size  > MAX_ASSOCIATIVITY || \
      config->L2_set_size  > MAX_ASSOCIATIVITY) 
  {
    fprintf(stderr, "Max set size (associativity) is %lu.\n", MAX_ASSOCIATIVITY);
    return false;
  }

  // Max Number of virtual pages
  if (config->pt_num_vpages > NUM_VPAGES_MAX) {
    fprintf(stderr, "PAGE TABLE Max number of virtual pages is %lu.\n", NUM_VPAGES_MAX);
    return false;
  }

  // Max Number of physical pages
  if (config->pt_num_ppages > NUM_PPAGES_MAX) {
    fprintf(stderr, "PAGE TABLE Max number of physical pages is %lu.\n", NUM_PPAGES_MAX);
    return false;
  }

  // Min Data line size
  if (config->dc_line_size < MIN_LINE_SIZE) {
    fprintf(stderr, "DC Min Line size is %lu.\n", MIN_LINE_SIZE);
    return false;
  }

  // L2 Line size should be >= DC line 
  if (config->L2_line_size < config->dc_line_size) {
    fprintf(stderr, "L2 Line size should be greater than or equal to DC line size");
    return false;
  }

  // Values that should be powers of 2
  if (!is_power2(config->tlb_set_size)) {
    fprintf(stderr, "TLB Line size should be a power of 2.\n");
    return false;
  } else if (!is_power2(config->dc_set_size)) {
    fprintf(stderr, "DC Set size should be a power of 2.\n");
    return false;
  } else if (!is_power2(config->dc_line_size)) {
    fprintf(stderr, "DC Line size should be a power of 2.\n");
    return false;
  } else if (!is_power2(config->L2_set_size)) {                       //! THIS RULE IS NOT STRICTLY STATED IN THE REQUIREMENTS
    fprintf(stderr, "L2 Set size should be a power of 2.\n");
    return false;
  } else if (!is_power2(config->L2_line_size)) {                      //! THIS RULE IS NOT STRICTLY STATED IN THE REQUIREMENTS
    fprintf(stderr, "L2 Line size should be a power of 2.\n");
    return false;
  } else if (!is_power2(config->pt_num_vpages)) {
    fprintf(stderr, "PAGE TABLE Number of virtual pages should be power of 2.\n");
    return false;
  } else if (!is_power2(config->pt_num_ppages)) {
    fprintf(stderr, "PAGE TABLE Number of physical pages should be power of 2.\n");
    return false;
  }

    return true;
}

Config* read_config(const char* filename) {
  FILE* f = fopen(filename, "r");
  if (!f) {
    perror("Failed to open config file");
    return NULL;
  }

  Config* config = malloc(sizeof(Config));
  char* buf = NULL;
  size_t buf_size;
  char c;

  // Data TLB configuration
  getline(&buf, &buf_size, f);
  if (strcmp(buf, "Data TLB configuration\n")) {
    fprintf(stderr, "Expected \"Data TLB configuration\" on line 1.\n");
    goto config_fail;
  }

  getline(&buf, &buf_size, f);
  if(sscanf(buf, "Number of sets: %lu", &config->tlb_num_sets) != 1) {
    fprintf(stderr, "Expected \"Number of sets: <num>\" on line 2.\n");
    goto config_fail;
  }

  getline(&buf, &buf_size, f);
  if(sscanf(buf, "Set size: %lu", &config->tlb_set_size) != 1) {
    fprintf(stderr, "Expected \"Set size: <num>\" on line 3.\n");
    goto config_fail;
  }

  if (fgetc(f) != (int)'\n') {
    fprintf(stderr, "[WARNING] Expected newline on line 4.\n");
  }

  // Page Table configuration
  getline(&buf, &buf_size, f);
  if (strcmp(buf, "Page Table configuration\n")) {
    fprintf(stderr, "Expected \"Page Table configuration\" on line 5.\n");
    goto config_fail;
  }

  getline(&buf, &buf_size, f);
  if(sscanf(buf, "Number of virtual pages: %lu", &config->pt_num_vpages) != 1) {
    fprintf(stderr, "Expected \"Number of virtual pages: <num>\" on line 6.\n");
    goto config_fail;
  }

  getline(&buf, &buf_size, f);
  if(sscanf(buf, "Number of physical pages: %lu", &config->pt_num_ppages) != 1) {
    fprintf(stderr, "Expected \"Number of physical pages: <num>\" on line 7.\n");
    goto config_fail;
  }

  getline(&buf, &buf_size, f);
  if(sscanf(buf, "Page size: %lu", &config->pt_page_size) != 1) {
    fprintf(stderr, "Expected \"Page size: <num>\" on line 8.\n");
    goto config_fail;
  }

  if (fgetc(f) != (int)'\n') {
    fprintf(stderr, "[WARNING] Expected newline on line 9.\n");
  }

  // Data Cache configuration
  getline(&buf, &buf_size, f);
  if (strcmp(buf, "Data Cache configuration\n")) {
    fprintf(stderr, "Expected \"Data Cache configuration\" on line 10.\n");
    goto config_fail;
  }

  getline(&buf, &buf_size, f);
  if(sscanf(buf, "Number of sets: %lu", &config->dc_num_sets) != 1) {
    fprintf(stderr, "Expected \"Number of sets: <num>\" on line 11.\n");
    goto config_fail;
  }

  getline(&buf, &buf_size, f);
  if(sscanf(buf, "Set size: %lu", &config->dc_set_size) != 1) {
    fprintf(stderr, "Expected \"Set size: <num>\" on line 12.\n");
    goto config_fail;
  }

  getline(&buf, &buf_size, f);
  if(sscanf(buf, "Line size: %lu", &config->dc_line_size) != 1) {
    fprintf(stderr, "Expected \"Line size: <num>\" on line 13.\n");
    goto config_fail;
  }

  getline(&buf, &buf_size, f);
  if(sscanf(buf, "Write through/no write allocate: %c", &c) != 1) {
    fprintf(stderr, "Expected \"Write through/no write allocate: <y,n>\" on line 14.\n");
    goto config_fail;
  }
  switch (c) {
    case 'y':
      config->dc_write = true;
      break;
    case 'n':
      config->dc_write = false;
      break;
    default:  
      fprintf(stderr, "Expected \"Write through/no write allocate: <y,n>\" on line 14.\n");
      goto config_fail;
  }

  if (fgetc(f) != (int)'\n') {
    fprintf(stderr, "[WARNING] Expected newline on line 15.\n");
  }

  // L2 Cache configuration
  getline(&buf, &buf_size, f);
  if (strcmp(buf, "L2 Cache configuration\n")) {
    fprintf(stderr, "Expected \"L2 Cache configuration\" on line 16.\n");
    goto config_fail;
  }

  getline(&buf, &buf_size, f);
  if(sscanf(buf, "Number of sets: %lu", &config->L2_num_sets) != 1) {
    fprintf(stderr, "Expected \"Number of sets: <num>\" on line 17.\n");
    goto config_fail;
  }

  getline(&buf, &buf_size, f);
  if(sscanf(buf, "Set size: %lu", &config->L2_set_size) != 1) {
    fprintf(stderr, "Expected \"Set size: <num>\" on line 18.\n");
    goto config_fail;
  }

  getline(&buf, &buf_size, f);
  if(sscanf(buf, "Line size: %lu", &config->L2_line_size) != 1) {
    fprintf(stderr, "Expected \"Line size: <num>\" on line 19.\n");
    goto config_fail;
  }

  getline(&buf, &buf_size, f);
  if(sscanf(buf, "Write through/no write allocate: %c", &c) != 1) {
    fprintf(stderr, "Expected \"Write through/no write allocate: <y,n>\" on line 20.\n");
    goto config_fail;
  }
  switch (c) {
    case 'y':
      config->L2_write = true;
      break;
    case 'n':
      config->L2_write = false;
      break;
    default:  
      fprintf(stderr, "Expected \"Write through/no write allocate: <y,n>\" on line 20.\n");
      goto config_fail;
  }

  if (fgetc(f) != (int)'\n') {
    fprintf(stderr, "[WARNING] Expected newline on line 21.\n");
  }

  // Toggles
  getline(&buf, &buf_size, f);
  if(sscanf(buf, "Virtual addresses: %c", &c) != 1) {
    fprintf(stderr, "Expected \"Virtual addresses: <y,n>\" on line 22.\n");
    goto config_fail;
  }
  switch (c) {
    case 'y':
      config->virtual_addresses = true;
      break;
    case 'n':
      config->virtual_addresses = false;
      break;
    default:  
      fprintf(stderr, "Expected \"Virtual addresses: <y,n>\" on line 22.\n");
      goto config_fail;
  }

  getline(&buf, &buf_size, f);
  if(sscanf(buf, "TLB: %c", &c) != 1) {
    fprintf(stderr, "Expected \"TLB: <y,n>\" on line 23.\n");
    goto config_fail;
  }
  switch (c) {
    case 'y':
      config->use_tlb= true;
      break;
    case 'n':
      config->use_tlb = false;
      break;
    default:  
      fprintf(stderr, "Expected \"TLB: <y,n>\" on line 23.\n");
      goto config_fail;
  }

  getline(&buf, &buf_size, f);
  if(sscanf(buf, "L2 cache: %c", &c) != 1) {
    fprintf(stderr, "Expected \"L2 cache: <y,n>\" on line 24.\n");
    goto config_fail;
  }
  switch (c) {
    case 'y':
      config->use_L2 = true;
      break;
    case 'n':
      config->use_L2 = false;
      break;
    default:  
      fprintf(stderr, "Expected \"L2: <y,n>\" on line 24.\n");
      goto config_fail;
  }
  

  // CONSTRAINT VALIDATION
  fprintf(stderr, "Checking constraint validations\n"); 
  if (!validate_config(config)) goto config_fail;

  return config;

config_fail:
  free(config);
  return NULL;
}
