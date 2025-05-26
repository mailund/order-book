#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "order_list_with_map.h"

// Keep capacity and load factors powers of two for the hash function!
#define INITIAL_CAPACITY 4
#define LOAD_FACTOR 8

// ---------- Internal Utilities ----------

static size_t hash(int key, size_t cap) {
  uint32_t x = (uint32_t)key;
  x *= 2654435761u;     // Knuth's multiplicative constant
  return x & (cap - 1); // cap is a power of two so this is a fast mod
}

static OrderIndexEntry *map_lookup(OrderArrayWithMap *arr, int key) {
  size_t h = hash(key, arr->map_capacity);
  for (size_t i = 0; i < arr->map_capacity; ++i) {
    size_t idx = (h + i) % arr->map_capacity;
    if (arr->map[idx].status == MAP_EMPTY)
      return NULL;
    if (arr->map[idx].status == MAP_OCCUPIED && arr->map[idx].key == key)
      return &arr->map[idx];
  }
  return NULL;
}

static OrderIndexEntry *map_probe_insert(OrderArrayWithMap *arr, int key) {
  size_t h = hash(key, arr->map_capacity);
  OrderIndexEntry *tombstone = NULL;
  for (size_t i = 0; i < arr->map_capacity; ++i) {
    size_t idx = (h + i) & (arr->map_capacity - 1);
    if (arr->map[idx].status == MAP_OCCUPIED && arr->map[idx].key == key)
      return &arr->map[idx];
    if (arr->map[idx].status == MAP_TOMBSTONE && !tombstone)
      tombstone = &arr->map[idx];
    if (arr->map[idx].status == MAP_EMPTY)
      return tombstone ? tombstone : &arr->map[idx];
  }
  return NULL;
}

static void map_insert(OrderArrayWithMap *arr, int key, size_t index) {
  OrderIndexEntry *entry = map_probe_insert(arr, key);
  assert(entry && "Map insert failed");
  entry->key = key;
  entry->index = index;
  entry->status = MAP_OCCUPIED;
}

static void map_update(OrderArrayWithMap *arr, int key, size_t new_index) {
  OrderIndexEntry *entry = map_lookup(arr, key);
  assert(entry && "Key not found in map for update");
  entry->index = new_index;
}

static void rehash(OrderArrayWithMap *arr) {
  memset(arr->map, 0, arr->map_capacity * sizeof(OrderIndexEntry));
  for (size_t i = 0; i < arr->size; ++i) {
    map_insert(arr, arr->data[i].order_id, i);
  }
}

// ---------- Initialization and Cleanup ----------

void init_order_array_with_map(OrderArrayWithMap *arr) {
  arr->size = 0;
  arr->capacity = INITIAL_CAPACITY;
  arr->data = malloc(arr->capacity * sizeof(Order));
  if (!arr->data) {
    perror("malloc data");
    exit(1);
  }

  arr->map_capacity = arr->capacity * LOAD_FACTOR;
  arr->map = calloc(arr->map_capacity, sizeof(OrderIndexEntry));
  if (!arr->map) {
    perror("calloc map");
    exit(1);
  }
}

void free_order_array_with_map(OrderArrayWithMap *arr) {
  free(arr->data);
  free(arr->map);
  arr->data = NULL;
  arr->map = NULL;
  arr->size = arr->capacity = arr->map_capacity = 0;
}

// ---------- Resize ----------

static void resize_order_array_with_map(OrderArrayWithMap *arr) {
  arr->capacity *= 2;
  arr->data = realloc(arr->data, arr->capacity * sizeof(Order));
  if (!arr->data) {
    perror("realloc data");
    exit(1);
  }

  arr->map_capacity = arr->capacity * LOAD_FACTOR;
  arr->map = realloc(arr->map, arr->map_capacity * sizeof(OrderIndexEntry));
  if (!arr->map) {
    perror("realloc map");
    exit(1);
  }

  memset(arr->map, 0, arr->map_capacity * sizeof(OrderIndexEntry));
  rehash(arr);
}

// ---------- Core Operations ----------

void append_order_with_map(OrderArrayWithMap *arr, Order order) {
  if (arr->size == arr->capacity) {
    resize_order_array_with_map(arr);
  }
  arr->data[arr->size] = order;
  map_insert(arr, order.order_id, arr->size);
  arr->size++;
}

Order *find_order_by_id(OrderArrayWithMap *arr, int order_id) {
  OrderIndexEntry *entry = map_lookup(arr, order_id);
  return entry ? &arr->data[entry->index] : NULL;
}

void remove_order_by_id(OrderArrayWithMap *arr, int order_id) {
  OrderIndexEntry *entry = map_lookup(arr, order_id);
  if (!entry)
    return;

  size_t remove_idx = entry->index;
  size_t last_idx = arr->size - 1;

  entry->status = MAP_TOMBSTONE;

  if (remove_idx != last_idx) {
    arr->data[remove_idx] = arr->data[last_idx];
    map_update(arr, arr->data[remove_idx].order_id, remove_idx);
  }

  arr->size--;
}

// ---------- Sorting ----------

static int cmp_asc(const void *a, const void *b) {
  const Order *o1 = a, *o2 = b;
  if (o1->price != o2->price)
    return o1->price - o2->price;
  return o1->quantity - o2->quantity;
}

static int cmp_desc(const void *a, const void *b) {
  const Order *o1 = a, *o2 = b;
  if (o1->price != o2->price)
    return o2->price - o1->price;
  return o2->quantity - o1->quantity;
}

void sort_orders_asc(OrderArrayWithMap *arr) {
  qsort(arr->data, arr->size, sizeof(Order), cmp_asc);
  rehash(arr);
}

void sort_orders_desc(OrderArrayWithMap *arr) {
  qsort(arr->data, arr->size, sizeof(Order), cmp_desc);
  rehash(arr);
}
