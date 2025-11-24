#pragma once
#include <stddef.h>
#include <stdbool.h>

typedef struct RStation RStation;

struct RStation {
  size_t* store;
  size_t index;
  size_t size;
  size_t capacity;
};

void rs_free(RStation* rs);
RStation* rs_new(size_t capacity);

size_t rs_peek(RStation* rs);

// replaces lowest value
// also returns previously lowest value
size_t rs_push(RStation* rs, size_t val);
