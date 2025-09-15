#include "util.h"
#include <limits.h>
#include <stdio.h>

#define BINARY_SPACING 8
void printb(size_t n) {
  for (int i = sizeof(unsigned long) * 8 - 1; i >= 0; i--) {
    fputc((n & (1UL << i)) ? '1' : '0', stdout);
    if (!(i % BINARY_SPACING)) fputc(' ', stdout);
  }
}

// assumes the number is already a power of 2
// returns ULONG_MAX on error
size_t log_2(size_t n) {
  for (size_t i = 0; i < sizeof(size_t) * 8; i++, n >>= 1) {
    if (n & 1) return i;
  }
  fprintf(stderr, "WARNING log_2 was given the number 0\n");
  return 0;
}
