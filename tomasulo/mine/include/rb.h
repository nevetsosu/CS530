#pragma once
#include <stddef.h>
#include <stdbool.h>

typedef struct RingBuffer RingBuffer;

struct RingBuffer {
  size_t* store;
  size_t size;
  size_t index;
  size_t capacity;
};

RingBuffer* rb_new(size_t capacity);
void rb_free(RingBuffer* rb);
void rb_push(RingBuffer* rb, size_t val);
bool rb_pop(RingBuffer* rb, size_t* val);
bool rb_peek(const RingBuffer* rb, size_t* val);
bool rb_full(const RingBuffer* rb);
