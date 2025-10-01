#include <stddef.h>

void printb(size_t n);

// assumes the number is already a power of 2
// returns ULONG_MAX on error
size_t log_2(size_t n);
