#pragma once

#include "order_array.h"
#include <stdbool.h>
#include <stddef.h>

#define EMPTY_KEY -1

typedef struct {
  int key;
  Order *value;
} OrderMapEntry;

typedef struct {
  OrderMapEntry *entries;
  size_t capacity;
  size_t size;
} OrderMap;

void order_map_init(OrderMap *map, size_t initial_capacity);
void order_map_free(OrderMap *map);
bool order_map_set(OrderMap *map, int key, Order *value);
Order *order_map_get(OrderMap *map, int key);
bool order_map_remove(OrderMap *map, int key);
