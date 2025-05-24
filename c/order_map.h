#pragma once

#include "order_array.h"
#include <stdbool.h>
#include <stddef.h>

#define EMPTY_KEY -1

typedef struct {
  int key;
  // We are not using direct pointers to Order * since they sit in pools that
  // might reallocate. Instead we get the OrderArray (that shouldn't move)
  // and index into it.
  OrderArray *pool;
  size_t index;
} OrderMapEntry;

typedef struct {
  OrderMapEntry *entries;
  size_t capacity;
  size_t size;
} OrderMap;

static inline Order *entry_as_pointer(OrderMapEntry *entry) {
  return &entry->pool->data[entry->index];
}

void order_map_init(OrderMap *map, size_t initial_capacity);
void order_map_free(OrderMap *map);
bool order_map_set(OrderMap *map, int key, OrderArray *arr, Order *value);
OrderMapEntry *order_map_get(OrderMap *map, int key);
bool order_map_remove(OrderMap *map, int key);

static inline Order *entry_as_order(OrderMapEntry *entry) {
  return entry ? entry_as_pointer(entry) : NULL;
}
static inline Order *order_map_get_order(OrderMap *map, int key) {
  OrderMapEntry *entry = order_map_get(map, key);
  if (entry) {
    return entry_as_pointer(entry);
  }
  return NULL; // Not found
}
