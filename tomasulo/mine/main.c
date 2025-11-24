#include <stdio.h>
#include <stdlib.h>
#include "config.h"
#include "instr.h"
#include "machine.h"

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

  // init machine
  State* state = machine_init(config);
  if (!state) {
    fprintf(stderr, "Failed to initialize machine\n");
    return 1;
  }

  // READING LOOP
  char* line = NULL;
  size_t size = 0;
  ssize_t len = 0;

  Instr* sentinel = instr_sentinel();
  Instr* cur = sentinel;
  while ((len = getline(&line, &size, stdin)) > 0) {
    line[len - 1] = '\0';
    cur->next = instr_parse(line);
    cur->next->prev = cur;
    cur = cur->next;
    
    machine_schedule(state, cur);
  }
  free(line);
  
  // FINAL RESULTS
  printf("                    Pipeline Simulation\n");
  printf("-----------------------------------------------------------\n");
  printf("                                    Memory Writes\n");
  printf("     Instruction      Issues Executes  Read  Result Commits\n");
  printf("--------------------- ------ -------- ------ ------ -------\n");
  
  cur = sentinel->next;
  while (cur != NULL) {
    InstrStats* stats = &cur->stats;
    printf("%-21s %6lu %3lu -%3lu %6lu %6lu %7lu\n", cur->str, stats->issue, stats->execute_start, stats->execute_end, stats->mem_read, stats->cdb_write, stats->commit);
    cur = cur->next;
  }
  
  const StateStats* stats = machine_stats(state);
  printf("\n\nDelays\n------\n");
  printf("reorder buffer delays: %lu\n", stats->reorder_buffer_delays);
  printf("reservation station delays: %lu\n", stats->reservation_station_delays);
  printf("data memory conflict delays: %lu\n", stats->data_memory_conflict_delays);
  printf("true dependence delays: %lu\n", stats->true_dependence_delays);

  // IGNORE CLEANING FOR NOW
  return 0;

  // CLEAN
  cur = sentinel;
  while (cur != NULL) {
    Instr* next = cur->next;
    instr_free(cur);
    cur = next;
  }

  config_free(config);
}
