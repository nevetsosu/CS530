#include <stdio.h>
#include <stdlib.h>
#include "machine.h" 
#include "instr.h"
#include "rs.h"
#include "rb.h"
#include "bitvec.h"

#define N_OPS 7

#define COMMIT_INIT_CAPACITY 2

typedef struct State State;
struct State {
  RStation* stations[N_OPS];
  size_t latencies[N_OPS];

  RingBuffer* reorder;
  BitVector* bv;
  StateStats stats;
};

static const enum op_t unique_stations[] = { STORE, ADD, FMUL, FADD, BRANCH };

void machine_free(State* state) { 
  for (size_t i = 0; i < sizeof(unique_stations) / sizeof(*unique_stations); i++) {
    rs_free(state->stations[unique_stations[i]]);
  }
/*
  rs_free(state->stations[STORE]);
  rs_free(state->stations[ADD]);
  rs_free(state->stations[FMUL]);
  rs_free(state->stations[FADD]);
  rs_free(state->stations[BRANCH]);
*/
  rb_free(state->reorder);
  bv_free(state->bv);
  free(state);
}

State* machine_init(const Config* config) {
  State* state = malloc(sizeof(*state));
  if (!state) return NULL;

  // store and load
  state->stations[STORE] = rs_new(config->eff_addr_buf);
  if (!state->stations[STORE]) goto machine_init_fail;
  state->latencies[STORE] = 1;

  // add and sub // for some reason this one can't be a single line assignment
  state->stations[SUB] = rs_new(config->fp_adds_buf);
  state->stations[ADD] = state->stations[SUB];
  if (!state->stations[ADD]) goto machine_init_fail;
  state->latencies[ADD] = 1;

  // fmul and div
  state->stations[FMUL] = state->stations[FDIV] = rs_new(config->fp_muls_buf);
  if (!state->stations[FMUL]) goto machine_init_fail;
  state->latencies[FMUL] = config->fp_mul_lat;
  state->latencies[FDIV] = config->fp_div_lat;

  // fadd and fsub
  state->stations[FADD] = state->stations[FSUB] = rs_new(config->ints_buf);
  if (!state->stations[FADD]) goto machine_init_fail;
  state->latencies[FADD] = config->fp_add_lat;
  state->latencies[FSUB] = config->fp_sub_lat;

  // exec portion of branches
  state->stations[BRANCH] = rs_new(1);
  if (!state->stations[BRANCH]) goto machine_init_fail;
  state->latencies[BRANCH] = 1;

  // reorder buffer
  state->reorder = rb_new(config->reorder_buf);
  if (!state->reorder) goto machine_init_fail;

  // commit bitset
  state->bv = bv_new(COMMIT_INIT_CAPACITY);

  return state;

machine_init_fail:
  if (state->stations[STORE])  rs_free(state->stations[STORE]);
  if (state->stations[ADD])    rs_free(state->stations[ADD]);
  if (state->stations[FMUL])   rs_free(state->stations[FMUL]);
  if (state->stations[FADD])   rs_free(state->stations[FADD]);
  if (state->stations[BRANCH]) rs_free(state->stations[BRANCH]); 
  if (state->reorder)          rb_free(state->reorder);
  if (state->bv)               bv_free(state->bv);
  if (state)                   free(state);
  return NULL;
} 

void machine_schedule(State* state, Instr* instr) {
  InstrStats* stats = &instr->stats;
  RStation* station = state->stations[instr->op_type];

  // Projected issue is just 1 after the previous issue
  size_t projected_issue = instr->prev->stats.issue + 1;

  // check ROB buffer for ISSUE delays
  size_t reorder_buffer_delay = 0;
  if (rb_full(state->reorder)) {
    size_t issue;
    rb_pop(state->reorder, &issue);
    issue += 1;
    
    printf("reorder buffer full, popping value: %lu\n", issue);
    if (issue > projected_issue)
      reorder_buffer_delay = issue - projected_issue;  
  }
  
  // check reservation station delay for ISSUE delays
  size_t reservation_station_delay = 0;
  size_t rs_avail = rs_peek(station);     // get next clock cycle a station will become available
  if (rs_avail > projected_issue) {
    size_t issue = rs_avail + 1;
    
    if (issue > projected_issue)
      reservation_station_delay = issue - projected_issue;
  }
  
  // calculate delay if any
  stats->issue = projected_issue;
  if (reservation_station_delay > reorder_buffer_delay) {
    state->stats.reservation_station_delays += reservation_station_delay;
    stats->issue += reservation_station_delay;
  }
  else {
    state->stats.reorder_buffer_delays += reorder_buffer_delay;
    stats->issue += reorder_buffer_delay;
  }

  // exec depends on data dependencies
  stats->execute_start = stats->issue + 1;
  stats->execute_end = stats->execute_start - 1 + state->latencies[instr->op_type];
  rs_push(station, stats->execute_end);

  // depends purely on CDB availability
  size_t projected_cdb_write = stats->execute_end + 1;
  stats->cdb_write = bv_insert(state->bv, projected_cdb_write);
  
  // depends on commit availability
  size_t next_avail_commit = instr->prev->stats.commit + 1;
  size_t projected_commit = stats->cdb_write + 1;
  stats->commit = (next_avail_commit > projected_commit) ? next_avail_commit : projected_commit;

  // commit
  rb_push(state->reorder, stats->commit);
}

StateStats* machine_stats(State* state) {
  return &state->stats;
}
