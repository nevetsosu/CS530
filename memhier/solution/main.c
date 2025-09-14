#include <stdio.h>
#include "config.h"

int main() {
  Config* config = read_config("../trace.config");
  if (!config) {
    fprintf(stderr, "Failed to read config.\n");
    return 1;
  }

  print_config(config);
  
  free_config(config);
}
