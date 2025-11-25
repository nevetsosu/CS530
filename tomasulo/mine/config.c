#include "config.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

void config_free(Config* config) {
  free(config);
}

Config* config_parse(const char* file_name) {
  char* line = NULL;
  size_t size = 0;

  // Open file
  FILE* f = fopen(file_name, "r");
  if (!f) { 
    perror("Failed to open config file.\n");
    return NULL;
  }

  Config* config = malloc(sizeof(*config));
  if (!config) {
    fprintf(stderr, "Failed to allocate config\n");
    goto config_parse_fail;
  }

  // Eat first two lines
  getline(&line, &size, f);
  getline(&line, &size, f);
  
  // eff addr buffer
  getline(&line, &size, f);
  sscanf(line, "eff addr:%u", &config->eff_addr_buf);
  
  // fp adds buffer
  getline(&line, &size, f);
  sscanf(line, "   fp adds:%u", &config->fp_adds_buf);
  
  // fp muls buffer
  getline(&line, &size, f);
  sscanf(line, "   fp muls:%u", &config->fp_muls_buf);
  
  // ints buffer 
  getline(&line, &size, f);
  sscanf(line, "ints:%u", &config->ints_buf);

  // reorder buffer 
  getline(&line, &size, f);
  sscanf(line, "reorder:%u", &config->reorder_buf);

  // Eat unecessarily lines
  getline(&line, &size, f); 
  getline(&line, &size, f);
  getline(&line, &size, f);

  // fp add latencies
  getline(&line, &size, f);
  sscanf(line, "   fp_add:%u", &config->fp_add_lat);
  
  // fp sub latencies
  getline(&line, &size, f);
  sscanf(line, "   fp_sub:%u", &config->fp_sub_lat);

  // fp mul latencies
  getline(&line, &size, f);
  sscanf(line, "   fp_mul:%u", &config->fp_mul_lat);

  // fp div latencies
  getline(&line, &size, f);
  sscanf(line, "   fp_div:%u", &config->fp_div_lat);
 
  goto config_parse_clean;

config_parse_fail:
  if (config) {
    config_free(config);
    config = NULL;
  }

config_parse_clean:
  if (f) fclose(f);
  if (line) free(line);

  return config;
}

void config_print(const Config* config) {
  printf("Configuration\n");
  printf("-------------\n");
  printf("buffers:\n");
  
  printf("   eff addr: %u\n", config->eff_addr_buf);
  printf("    fp adds: %u\n" , config->fp_adds_buf);
  printf("    fp muls: %u\n",  config->fp_muls_buf);
  printf("       ints: %u\n",     config->ints_buf);
  printf("    reorder: %u\n",  config->reorder_buf);

  printf("\nlatencies:\n");

  printf("   fp add: %u\n",   config->fp_add_lat);
  printf("   fp sub: %u\n",   config->fp_sub_lat);
  printf("   fp mul: %u\n",   config->fp_mul_lat);
  printf("   fp div: %u\n",   config->fp_div_lat);
  printf("\n\n");
}
