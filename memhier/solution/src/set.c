#include <stdlib.h>
#include <stddef.h>
#include "set.h"

// move this to header
struct SetNode {
  void* data;   
  SetNode* next;
  SetNode* prev;
};

struct Set {
  size_t size;
  SetNode* node_list;   // the first node should always be considered the sentinel
};

// Connect the two nodes by setting their pointers
void _set_connect(SetNode* prev, SetNode* next) {
  prev->next = next;
  next->prev = prev;
}

// Disconnect the node from the chain 
// Reforms the connect between the left and right node
// Returns the same node
SetNode* _set_disconnect(SetNode* node) {
  SetNode* prev = node->prev;
  SetNode* next = node->next;

  // Connect the lfet and right nodes
  prev->next = next;
  prev->prev = prev;

  // null out current node pointers
  node->next = NULL;
  node->prev = NULL; 

  return node;
}

// Insert the node `new` on the RIGHT of the `target` node.
// Effectively, `new` gets inserted BETWEEN target and target's next.
void _set_insert_right(SetNode* new, SetNode* target) {
  SetNode* next = target->next;

  _set_connect(target, new);
  _set_connect(new, next);
}

// Insert the node `new` on the LEFT of the `target` node
// Effectively, `new` gets inserted BETWEEN target's prev and target.
void _set_insert_left(SetNode* new, SetNode* target) {
  SetNode* prev = target->prev;
  
  _set_connect(prev, new);
  _set_connect(new, target);
}

// returns a new Set object with size `size`.
// the size does NOT include the sentinel node
Set* set_new(size_t size) {
  Set* set = malloc(sizeof(Set));
  set->node_list = malloc(sizeof(SetNode) * size + 1);

  // the first node is always the implied sentinel
  SetNode* sentinel = set->node_list;
  sentinel->prev = sentinel->next = sentinel;

  // NOTE: size here doesn't consider the sentinel node
  // This loop doesn't impact the very last node in the array
  size_t i;
  for (i = 0; i < set->size; i++) {
    _set_connect(set->node_list + i, set->node_list + i + 1);
  }

  // connect the last node to the sentinel
  _set_connect(set->node_list + set->size, set->node_list);

  return set;
}

void set_free(Set* set) {
  free(set->node_list);
  free(set);
}

// Returns the most recent node
SetNode* set_get_mru(Set* set) {
  return set->node_list->next;
}

// Returns the least recent node
SetNode* set_get_lru(Set* set) {
  return set->node_list->prev;
}

// disconnects the node `node` from its current location in the list and puts it in the MRU position.
void set_assign_mru(Set* set, SetNode* node) {
  _set_disconnect(node);
  _set_insert_right(node, set->node_list);
} 

// disconnects the node `node` from its current location in the list and puts it in the LRU position. 
// This is unlikely to be used in this project but I put it here for API completion. 
void set_assign_lru(Set* set, SetNode* node) {
  _set_disconnect(node);
  _set_insert_left(node, set->node_list);
}

// Returns the sentinel node if someone needs it for some reason
SetNode* set_sentinel(Set* set) {
  return set->node_list;
}

// Returns the data pointer contained in the struct
void* setnode_data(SetNode* node) {
  return node->data;
}
