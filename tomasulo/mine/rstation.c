#include "rstation.h"
#include <stdlib.h>

void rs_free(RStation* rs) {
  free(rs->store);
  free(rs);
}

RStation* rs_new(size_t capacity) {
  RStation* rs = malloc(sizeof(*rs));
  if (!rs) { return NULL; }

  rs->store = calloc(capacity, sizeof(*rs->store));
  if (!rs->store) { free(rs); return NULL; }

  rs->capacity = capacity;

  return rs;
}

// searches for the minimum value and sets the internal cursor to it.
// returns the lowest value.
size_t rs_peek(RStation* rs) {
  size_t min_index = 0;
  size_t min = (size_t) -1;

  for (size_t i = 0; i < rs->capacity; i++) {
    if (rs->store[i] < min) {
      min = rs->store[i];
      min_index = i;
    }
  }

  rs->index = min_index;
  return min;
}

// replaces the current index
// usually preceeded with a call to rs_min
size_t rs_push(RStation* rs, size_t val) {
  size_t prev_val = rs->store[rs->index];
  rs->store[rs->index] = val;
  
  return prev_val;
}
