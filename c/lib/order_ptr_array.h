#pragma once

#include "order.h"
#include <stddef.h>

// OrderPtrArray holds an array of pointers to Order objects

typedef struct {
  Order **data;
  size_t size;
  size_t capacity;
} OrderPtrArray;

void init_order_ptr_array(OrderPtrArray *arr);
void free_order_ptr_array(OrderPtrArray *arr);
void append_order_ptr(OrderPtrArray *arr, Order *order);
Order *order_ptr_at_index(const OrderPtrArray *arr, size_t index);
Order *order_ptr_by_id(const OrderPtrArray *arr, int order_id);
void remove_order_ptr_by_id(OrderPtrArray *arr, int order_id);
