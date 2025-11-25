#include <stdio.h>
#include <stdlib.h>
#include "machine.h" 
#include "instr.h"
#include "rs.h"
#include "rb.h"
#include "bitvec.h"
#include "config.h"

#define N_OPS 9

#define BV_INIT_CAPACITY 2

typedef struct State State;
struct State {
  RStation* stations[N_OPS];
  size_t latencies[N_OPS];

  RingBuffer* reorder;
  BitVector* mem_bv;
  BitVector* commit_bv;
  StateStats stats;
  const Config* config;
};

static inline size_t max(size_t a, size_t b) {
  return a > b ? a : b;
}

static const enum op_t unique_stations[] = { STORE, ADD, FMUL, FADD };

// traverses backwards through instructions and tries to find an instruction that writes to
// the current register and which cycle it does it on
static size_t _machine_data_dependency_search(State* state, Instr* instr, size_t operand) {
  size_t i = 0;
  Instr* cur = instr->prev;
  
  bool fp_type_reg;
  switch (instr->op_type) {
    case LOAD:
      /* FALLTHROUGH */
    case STORE:
      fp_type_reg = false;
      break;
    default:
      fp_type_reg = instr->fp;
  }

  // ignore if x0
  if (!fp_type_reg && !operand)
    return 0;
  
  // walk
  while (i < state->config->reorder_buf && cur != instr_sentinel()) {
    if (cur->fp != fp_type_reg ||
        cur->op_type == STORE  ||
        cur->op1 != operand) goto _machine_data_dependency_search_iterate;
    
    fprintf(stderr, "\tdependency on instruction: %s (cdb: %lu)\n", cur->str, cur->stats.cdb_write);
    return cur->stats.cdb_write;

_machine_data_dependency_search_iterate:
    i += 1;
    cur = cur->prev;
  }

  return 0;
}

// checks for data dependencies that would stall the current instruction
static size_t _machine_data_dependency(State* state, Instr* instr) {
  // all operations have to atleast have op2 checked
  size_t dependent_cycle = _machine_data_dependency_search(state, instr, instr->op2);  
  
  // determine what other operands need to be checked
  switch (instr->op_type) {
    // LOADS and STORES will have their own checks
    case LOAD:
      /* FALLTHROUGH */
    case STORE:
      break;

    default:
      // non-load stores also need op3 checked
      dependent_cycle = max(dependent_cycle, _machine_data_dependency_search(state, instr, instr->op3));
      break;
  }
  
  return dependent_cycle;
}

static size_t _machine_store_dependency(State* state, Instr* instr) {
  if (instr->op_type != LOAD) return 0;
  if (!instr->op3 && !instr->fp) return 0;    // ignore x0

  size_t i = 0;
  Instr* cur = instr->prev;

  while (i < state->config->reorder_buf && cur != instr_sentinel()) {
    if (cur->fp != instr->fp ||
        cur->op3 != instr->op3 ||
        cur->op_type != STORE) goto _machine_store_dependency_iterate;

    fprintf(stderr, "\tSTORE dependency on instruction: %s(commit: %lu)\n", cur->str, cur->stats.commit);
    return cur->stats.commit;

_machine_store_dependency_iterate:
    i += 1;
    cur = cur->prev;
  }

  return 0;
} 

void machine_free(State* state) { 
  for (size_t i = 0; i < sizeof(unique_stations) / sizeof(*unique_stations); i++) {
    rs_free(state->stations[unique_stations[i]]);
  }

  rb_free(state->reorder);
  bv_free(state->commit_bv);
  bv_free(state->mem_bv);
  free(state);
}

State* machine_init(const Config* config) {
  State* state = malloc(sizeof(*state));
  if (!state) return NULL;

  // store and load
  state->stations[STORE] = state->stations[LOAD] = rs_new(config->eff_addr_buf);
  if (!state->stations[STORE]) goto machine_init_fail;
  state->latencies[STORE] = state->latencies[LOAD] = 1;

  // add, sub, branch // for some reason this one can't be a single line assignment
  state->stations[SUB] = rs_new(config->ints_buf);
  state->stations[ADD] = state->stations[SUB];
  state->stations[BRANCH] = state->stations[ADD];
  if (!state->stations[ADD]) goto machine_init_fail;
  state->latencies[ADD] = state->latencies[SUB] = state->latencies[BRANCH] = 1;
  

  // fmul and div
  state->stations[FMUL] = state->stations[FDIV] = rs_new(config->fp_muls_buf);
  if (!state->stations[FMUL]) goto machine_init_fail;
  state->latencies[FMUL] = config->fp_mul_lat;
  state->latencies[FDIV] = config->fp_div_lat;

  // fadd and fsub
  state->stations[FADD] = state->stations[FSUB] = rs_new(config->fp_adds_buf);
  if (!state->stations[FADD]) goto machine_init_fail;
  state->latencies[FADD] = config->fp_add_lat;
  state->latencies[FSUB] = config->fp_sub_lat;

  // reorder buffer
  state->reorder = rb_new(config->reorder_buf);
  if (!state->reorder) goto machine_init_fail;

  // commit bitset
  state->commit_bv = bv_new(BV_INIT_CAPACITY);

  // mem bitset
  state->mem_bv = bv_new(BV_INIT_CAPACITY);

  state->config = config;

  return state;

machine_init_fail:
  for (size_t i = 0; i < sizeof(unique_stations) / sizeof(*unique_stations); i++) {
    if (state->stations[unique_stations[i]]) rs_free(state->stations[unique_stations[i]]);
  }
  if (state->reorder)          rb_free(state->reorder);
  if (state->commit_bv)        bv_free(state->commit_bv);
  if (state)                   free(state);
  return NULL;
} 

void machine_schedule(State* state, Instr* instr) {
  InstrStats* stats = &instr->stats;
  RStation* station = state->stations[instr->op_type];

  fprintf(stderr, "instruction: %s\n", instr->str);

  // PROJECTED ISSUE is just 1 after the previous issue
  size_t projected_issue = instr->prev->stats.issue + 1;
  fprintf(stderr, "\tprojected issue: %lu\n", projected_issue);

  // ROB DELAY: check ROB buffer for ISSUE delays
  size_t reorder_buffer_delay = 0;
  if (rb_full(state->reorder)) {
    size_t issue;
    rb_pop(state->reorder, &issue);
    
    fprintf(stderr, "\treorder buffer full: %lu\n", issue);
    if (issue > projected_issue)
      reorder_buffer_delay = issue - projected_issue;  
  }
  
  // RESERVATION STATION DELAY: check reservation station delay for ISSUE delays
  size_t reservation_station_delay = 0;
  size_t rs_avail = rs_peek(station);     // get next clock cycle a station will become available
  fprintf(stderr, "\trs_peek: %lu\n", rs_avail); 
  if (rs_avail >= projected_issue) {
    size_t issue = rs_avail + 1; 
    if (issue > projected_issue)
      reservation_station_delay = issue - projected_issue;
  }
  
  // RESERVATION STATION OR REORDER BUFFER DELAY: calculate delay if any
  stats->issue = projected_issue;
  if (reservation_station_delay > reorder_buffer_delay) {
    state->stats.reservation_station_delays += reservation_station_delay;
    stats->issue += reservation_station_delay;
  }
  else {
    state->stats.reorder_buffer_delays += reorder_buffer_delay;
    stats->issue += reorder_buffer_delay;
  }

  // EXEC_START: depends on data dependencies
  size_t projected_execute_start = stats->issue + 1;
  size_t data_dependent_start = _machine_data_dependency(state, instr) + 1;
  
  if (data_dependent_start > projected_execute_start) {
    instr->stats.execute_start = data_dependent_start;
    state->stats.true_dependence_delays += data_dependent_start - projected_execute_start;
  }
  else
    instr->stats.execute_start = projected_execute_start;
  
  // EXEC_END
  stats->execute_end = stats->execute_start - 1 + state->latencies[instr->op_type];
 
  // MEM_READ: memory depends on instruction and mem availability
  size_t projected_mem_read = stats->execute_end + 1;
  size_t store_dependent_start = _machine_store_dependency(state, instr);     // returns 0 if non-load instruction
  
  switch (instr->op_type) {
    case LOAD:
      if (store_dependent_start > projected_mem_read) {
        state->stats.true_dependence_delays += store_dependent_start - projected_mem_read;
        stats->mem_read = bv_insert(state->mem_bv, store_dependent_start);

        state->stats.data_memory_conflict_delays += stats->mem_read - store_dependent_start;
      }
      else {
        stats->mem_read = bv_insert(state->mem_bv, projected_mem_read);
        state->stats.data_memory_conflict_delays += stats->mem_read - projected_mem_read;
      }

      break;
    default:
      stats->mem_read = 0;
      break;
  }

  // FU RELEASE: when to release the station depends on the operation
  switch (instr->op_type) {
    case LOAD:
      rs_push(station, stats->mem_read);
      break;
    default:
      rs_push(station, stats->execute_end);
      break;
  }

  // CDB_WRITE depends instruction type AND CDB availability
  switch (instr->op_type) {
    case STORE:
      /* FALLTHROUGH */
    case BRANCH:
      stats->cdb_write = 0;
      break;
    default:
    stats->cdb_write = bv_insert(state->commit_bv, (stats->mem_read ? stats->mem_read : stats->execute_end) + 1); 
    break;
  }

  // COMMIT depends purely on commit availability
  size_t next_avail_commit = instr->prev->stats.commit + 1;
  size_t projected_commit = stats->cdb_write + 1;
  stats->commit = (next_avail_commit > projected_commit) ? next_avail_commit : projected_commit;

  // also make sure to take availability on the load/store functional unit during a store commit
  if (instr->op_type == STORE)
    bv_insert(state->mem_bv, stats->commit);

  // push commit
  rb_push(state->reorder, stats->commit);
}

StateStats* machine_stats(State* state) {
  return &state->stats;
}
