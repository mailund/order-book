#include "order_map.h"
#include <stdlib.h>
#include <string.h>

static inline size_t next_power_of_two(size_t x) {
  size_t power = 1;
  while (power < x)
    power <<= 1;
  return power;
}

static inline size_t hash_bin(int key, size_t capacity) {
  return (unsigned int)key & (capacity - 1);
}

void order_map_init(OrderMap *map, size_t initial_capacity) {
  map->capacity = next_power_of_two(initial_capacity);
  map->size = 0;
  map->entries = calloc(map->capacity, sizeof(OrderMapEntry));
  for (size_t i = 0; i < map->capacity; ++i)
    map->entries[i].key = EMPTY_KEY;
}

void order_map_free(OrderMap *map) {
  free(map->entries);
  map->entries = NULL;
  map->size = map->capacity = 0;
}

static void order_map_resize(OrderMap *map) {
  size_t old_capacity = map->capacity;
  OrderMapEntry *old_entries = map->entries;

  order_map_init(map, old_capacity * 2);

  for (size_t i = 0; i < old_capacity; ++i) {
    if (old_entries[i].key != EMPTY_KEY) {
      order_map_set(map, old_entries[i].key, old_entries[i].pool,
                    &old_entries[i].pool->data[old_entries[i].index]);
    }
  }

  free(old_entries);
}

bool order_map_set(OrderMap *map, int key, OrderArray *arr, Order *value) {
  if (map->size * 2 >= map->capacity)
    order_map_resize(map);

  size_t idx = hash_bin(key, map->capacity);
  while (map->entries[idx].key != EMPTY_KEY && map->entries[idx].key != key) {
    idx = (idx + 1) & (map->capacity - 1);
  }

  if (map->entries[idx].key == EMPTY_KEY)
    map->size++;

  map->entries[idx].key = key;
  map->entries[idx].pool = arr;
  map->entries[idx].index = pointer_index(arr, value);
  return true;
}

OrderMapEntry *order_map_get(OrderMap *map, int key) {
  size_t idx = hash_bin(key, map->capacity);
  size_t start = idx;
  while (map->entries[idx].key != EMPTY_KEY) {
    if (map->entries[idx].key == key) {
      return &map->entries[idx];
    }
    idx = (idx + 1) & (map->capacity - 1);
    if (idx == start)
      break;
  }
  return NULL; // Not found
}

bool order_map_remove(OrderMap *map, int key) {
  size_t idx = hash_bin(key, map->capacity);
  while (map->entries[idx].key != EMPTY_KEY) {
    if (map->entries[idx].key == key) {
      map->entries[idx].key = EMPTY_KEY;
      map->size--;
      return true;
    }
    idx = (idx + 1) & (map->capacity - 1);
  }
  return false;
}
