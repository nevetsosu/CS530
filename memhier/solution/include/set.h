#include <stddef.h>
#include <stdint.h>

//move this to header
#define SET_TRAVERSE_RIGHT(cur, sentinel) for (cur = sentinel->next; cur != sentinel; cur = cur->next) 
#define SET_TRAVERSE_LEFT(cur, sentinel) for (cur = sentinel->prev; cur != sentinel; cur = cur->prev)
#define SET_TRAVERSE_FLAT(index, set) for (index = 0; index < set->size; index++)

// move this to header
typedef struct SetNode SetNode;
typedef struct Set Set;

struct SetNode {
  void* data;   
  SetNode* next;
  SetNode* prev;
};

struct Set {
  size_t size;
  SetNode* node_list;   // the first node should always be considered the sentinel
};

Set* Set_new(const size_t size);
void Set_free(Set* set);

SetNode* Set_get_mru(const Set* set);
SetNode* Set_get_lru(const Set* set);
void Set_set_mru(Set* set, SetNode* node);
void Set_set_lru(Set* set, SetNode* node);
