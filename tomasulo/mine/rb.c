#include <stdlib.h>
#include "rb.h"

RingBuffer* rb_new(size_t capacity) {
  RingBuffer* rb = malloc(sizeof(*rb));
  rb->store = malloc(sizeof(*rb->store) * capacity);
  
  rb->size = 0;
  rb->index = 0;
  rb->capacity = capacity;

  return rb;

}
void rb_free(RingBuffer* rb) {
  free(rb->store);
  free(rb);
}

void rb_push(RingBuffer* rb, size_t val) {
  rb->store[(rb->index + rb->size) % rb->capacity] = val;

  if (rb->size == rb->capacity)
    rb->index = (rb->index + 1) % rb->capacity;
  else
    rb->size += 1;
}

bool rb_pop(RingBuffer* rb, size_t* val) {
  if (!rb->size) return false;
  
  *val = rb->store[rb->index];
  rb->index = (rb->index + 1) % rb->capacity;
  rb->size -= 1;

  return true;
}

bool rb_peek(const RingBuffer* rb, size_t* val) {
    if (!rb->size) return false;

    *val = rb->store[(rb->index + rb->size - 1) % rb->capacity];
    return true;
}

bool rb_full(const RingBuffer* rb) {
  return (rb->size == rb->capacity);
}

