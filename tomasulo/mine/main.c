#include <stdio.h>
#include "config.h"
#include "instr.h"

int main(int argc, char** argv) {
  char* conf_f = "config.txt";

  if (argc > 1) {
    conf_f = argv[1];
  }

  // Parse configuration file
  Config* config = config_parse(conf_f);
  if (!config) {
    fprintf(stderr, "Failed to get config\n");
    return 1;
  }

  // Print config
  config_print(config);

  // READING LOOP
  char* line = NULL;
  size_t size = 0;

  while (getline(&line, &size, stdin) > 0) {
    instr_parse(line);
  }
  //
  
  
  // go through and mark dependencies
  printf("                    Pipeline Simulation\n");
  printf("-----------------------------------------------------------\n"); 
}
