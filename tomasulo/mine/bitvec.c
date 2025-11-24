#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <string.h>
#include "bitvec.h"

struct BitVector {
  size_t* store;
  size_t capacity;
};

static const size_t bitsize = sizeof(size_t) * 8;

static void _bv_resize(BitVector* bv, size_t new_capacity) {
  bv->store = realloc(bv->store, new_capacity * sizeof(*bv->store));
  if (!bv->store) { fprintf(stderr, "failed to resize in _bv_resize"); bv->capacity = 0; return; }
  
  // zero out the new region
  memset(bv->store + bv->capacity, 0, (new_capacity - bv->capacity) * sizeof(*bv->store));

  bv->capacity = new_capacity;
}

static bool _bv_index(BitVector* bv, size_t index, size_t bit_pos) {
  return (bv->store[index] >> bit_pos) & 1;
}

void bv_free(BitVector* bv) {
  free(bv->store);
  free(bv);
}

BitVector* bv_new(size_t initial_capacity) {
  BitVector* bv = malloc(sizeof(*bv));
  if (!bv) { return NULL; }

  bv->store = calloc(1, sizeof(*bv->store));
  if (!bv->store) { free(bv); return NULL; }

  bv->capacity = initial_capacity;

  return bv;
}

size_t bv_insert(BitVector* bv, size_t pos) {
  size_t index = pos / bitsize;
  size_t bit_pos = pos % bitsize;

  // resize if needed
  if (index >= bv->capacity) {
    fprintf(stderr, "resizing for instruction position: %lu\n", pos);
    _bv_resize(bv, index * 2);
  }
  
  // walk through the bitset until there is an empty position
  for (size_t i = index; i < bv->capacity; i++) {
    for (size_t j = bit_pos; j < bitsize; j++) {
      if (_bv_index(bv, i, j)) {
        fprintf(stderr, "position: %lu taken\n", i * sizeof(size_t) * 8 + j);
        continue;
      }

      fprintf(stderr, "found position at %lu, index: %lu, bit pos: %lu\n", i * sizeof(size_t) * 8 + j, i, j);

      // set position and return position
      bv->store[i] |= (1lu << j);
      pos = i * bitsize + j;
      return pos;
    }
  }

  fprintf(stderr, "WARNING, bv_insert didn't find a place to insert\n");

  return 0;
}
