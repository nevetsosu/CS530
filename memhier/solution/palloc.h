#include <stddef.h>

typedef struct PAlloc PAlloc;

PAlloc* palloc_new(size_t num_pages);
void palloc_free(PAlloc* palloc);
void palloc_clear_page(PAlloc* palloc, size_t page);
size_t palloc_new_page(PAlloc* palloc);
