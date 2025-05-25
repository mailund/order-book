#pragma once

#include <assert.h>
#include <stdbool.h>
#include <stdlib.h>

#include "order.h"

typedef struct {
  Order *data;
  size_t size;
  size_t capacity;
} OrderArray;

void init_order_array(OrderArray *arr);
void free_order_array(OrderArray *arr);
void append_order(OrderArray *arr, Order order);
Order *order_at_index(const OrderArray *arr, size_t index);
Order *order_by_id(const OrderArray *arr, int order_id);
void remove_by_index(OrderArray *arr, size_t index);
void remove_by_id(OrderArray *arr, int order_id);

// NB: Be *super* careful if you use these on an array that changes
// size, since the array might be reallocated.
static inline bool points_into(const OrderArray *arr, const Order *order) {
  return !(order < arr->data || order >= arr->data + arr->size);
}
static inline size_t pointer_index(const OrderArray *arr, const Order *order) {
  assert(points_into(arr, order));
  return order - arr->data; // Calculate index based on pointer arithmetic
}
