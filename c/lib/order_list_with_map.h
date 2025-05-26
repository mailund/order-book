#pragma once

#include <stdbool.h>
#include <stddef.h>

#include "order.h"

typedef enum { MAP_EMPTY, MAP_OCCUPIED, MAP_TOMBSTONE } MapSlotStatus;

typedef struct {
  int key; // order_id
  Order *order_ptr;
  MapSlotStatus status;
} OrderIndexEntry;

typedef struct {
  Order **data; // dynamic array of Order*
  size_t size;
  size_t capacity;

  OrderIndexEntry *map; // hash map: order_id → Order*
  size_t map_capacity;
} OrderArrayWithMap;

void init_order_array_with_map(OrderArrayWithMap *arr);
void free_order_array_with_map(OrderArrayWithMap *arr);
void append_order_with_map(OrderArrayWithMap *arr, Order *order);
Order *find_order_by_id(OrderArrayWithMap *arr, int order_id);
void remove_order_by_id(OrderArrayWithMap *arr, int order_id);
void sort_orders_asc(OrderArrayWithMap *arr);
void sort_orders_desc(OrderArrayWithMap *arr);