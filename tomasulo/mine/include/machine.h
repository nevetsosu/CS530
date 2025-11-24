#pragma once
#include <stddef.h>
#include "config.h"
#include "instr.h"

typedef struct State State;
typedef struct StateStats StateStats;
struct State;

struct StateStats {
  size_t reorder_buffer_delays;
  size_t reservation_station_delays;
  size_t data_memory_conflict_delays;
  size_t true_dependence_delays;
};

State* machine_init(const Config* config);
void machine_schedule(State* state, Instr* instr);
StateStats* machine_stats(State* state);
