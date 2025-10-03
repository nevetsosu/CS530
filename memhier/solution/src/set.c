#include <stdlib.h>
#include <stdio.h>
#include "set.h"

// Connect the two nodes by Setting their pointers
void _Set_connect(SetNode* prev, SetNode* next) {
  prev->next = next;
  next->prev = prev;
}

// Disconnect the node from the chain 
// Reforms the connect between the left and right node
// Returns the same node
void _Set_disconnect(SetNode* node) {
  SetNode* prev = node->prev;
  SetNode* next = node->next;

  // Connect the lfet and right nodes
  prev->next = next;
  prev->prev = prev;

  // null out current node pointers
  node->next = NULL;
  node->prev = NULL; 
}

// Insert the node `new` on the RIGHT of the `target` node.
// Effectively, `new` gets inserted BETWEEN target and target's next.
void _Set_insert_right(SetNode* new, SetNode* target) {
  SetNode* next = target->next;

  _Set_connect(new, next);
  _Set_connect(target, new);
}

// Insert the node `new` on the LEFT of the `target` node
// Effectively, `new` gets inserted BETWEEN target's prev and target.
void _Set_insert_left(SetNode* new, SetNode* target) {
  SetNode* prev = target->prev;
  
  _Set_connect(prev, new);
  _Set_connect(new, target);
}

// returns a new Set object with size `size`.
// the size does NOT include the sentinel node
Set* Set_new(size_t size) {
  Set* set = malloc(sizeof(Set));
  set->node_list = malloc(sizeof(SetNode) * (size + 1));

  // the first node is always the implied sentinel
  SetNode* sentinel = set->node_list;
  sentinel->prev = sentinel->next = sentinel;

  // NOTE: size here doesn't consider the sentinel node
  // This loop doesn't impact the very last node in the array
  size_t i;
  for (i = 0; i < size; i++) {
    _Set_connect(set->node_list + i, set->node_list + i + 1);
  }

  // connect the last node to the sentinel
  _Set_connect(set->node_list + size, set->node_list);

  // set member vars
  set->size = size;

  return set;
}

void Set_free(Set* set) {
  free(set->node_list);
  free(set);
}

// Returns the most recent node
SetNode* Set_get_mru(const Set* set) {
  return set->node_list->next;
}

// Returns the least recent node
SetNode* Set_get_lru(const Set* set) {
  return set->node_list->prev;
}

// disconnects the node `node` from its current location in the list and puts it in the MRU position.
void Set_set_mru(Set* set, SetNode* node) {
  _Set_disconnect(node);
  _Set_insert_right(node, set->node_list);
} 

// disconnects the node `node` from its current location in the list and puts it in the LRU position. 
// This is unlikely to be used in this project but I put it here for API completion. 
void Set_set_lru(Set* set, SetNode* node) {
  _Set_disconnect(node);
  _Set_insert_left(node, set->node_list);
}

size_t Set_size(const Set* set) {
  return set->size;
}
