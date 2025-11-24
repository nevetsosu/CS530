#pragma once

typedef struct Config {
  unsigned int eff_addr_buf;
  unsigned int fp_adds_buf;
  unsigned int fp_muls_buf;
  unsigned int ints_buf;
  unsigned int reorder_buf;

  unsigned int fp_add_lat;
  unsigned int fp_sub_lat;
  unsigned int fp_mul_lat;
  unsigned int fp_div_lat;
} Config;

void config_free(Config* config);
Config* config_parse(const char* file_name);
void config_print(const Config* config);
