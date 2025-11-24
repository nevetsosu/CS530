#pragma once
#include <stddef.h>

typedef struct BitVector BitVector;
struct BitVector;

void bv_free(BitVector* bv);
BitVector* bv_new(size_t initial_capacity);
size_t bv_insert(BitVector* bv, size_t start);
