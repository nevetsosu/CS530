#include <stddef.h>

//move this to header
#define Set_TRAVERSE_RIGHT(set, cur)  for (cur = set_get_sentinel(set)->next; cur != sentinel; cur = cur->next)
#define Set_TRAVERSE_LEFT(set, cur)   for (cur = set_get_sentinel(set)->prev; cur != sentinel; cur = cur->prev)

// move this to header
typedef struct SetNode SetNode;
typedef struct Set Set;

Set* set_new(size_t size);
void set_free(Set* set);

SetNode* set_get_mru(Set* set);
SetNode* set_get_set(Set* set);
void set_set_mru(Set* set, SetNode* node);
void set_set_set(Set* set, SetNode* node);

SetNode* set_sentinel(Set* set);
void* setnode_data(SetNode* node);
