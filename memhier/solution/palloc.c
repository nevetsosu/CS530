#include <stdbool.h>
#include <stdlib.h>
#include <stdio.h>

struct PAlloc {
  size_t cur_pos;
  bool* pages;        // true for taken, false for free
  size_t num_pages;
  size_t pages_allocated;
};

typedef struct PAlloc PAlloc;

PAlloc* palloc_new(size_t num_pages) {
  PAlloc* palloc = malloc(sizeof(PAlloc));
  palloc->cur_pos = 0;
  palloc->pages_allocated = 0;
  palloc->num_pages = num_pages;
  palloc->pages = calloc(num_pages, sizeof(size_t));

  return palloc;
}

void palloc_free(PAlloc* palloc) {
  free(palloc->pages);
  free(palloc);
}

void palloc_clear_page(PAlloc* palloc, size_t page) {
  if (palloc->pages[page]) {
    palloc->pages_allocated -= 1;
    palloc->pages[page] = false;
  }
}

size_t palloc_new_page(PAlloc* palloc) {
  // try to find a free page
  
  if (palloc->pages_allocated < palloc->num_pages) {
    size_t count = 0;
    while (count < palloc->num_pages) {
      if (palloc->pages[palloc->cur_pos]) {
        palloc->cur_pos = (palloc->cur_pos + 1) % palloc->num_pages;
        continue;
      }
      size_t free_pos = palloc->cur_pos;

      palloc->pages_allocated += 1;
      palloc->cur_pos = (palloc->cur_pos + 1) % palloc->num_pages;

      return free_pos;
    }

    fprintf(stderr, "There are pages available, but couldn't be found??\n");
  }

  fprintf(stderr, "[PALLOC] Calling the Evictor, but its not currently implemented, so returning page 0\n"); 

  // else, call the evictor
  //
  // return free page from evictor
  
  return 0;
}
