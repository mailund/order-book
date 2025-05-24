#pragma once

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
Order *order_at_index(OrderArray *arr, size_t index);
Order *order_by_id(OrderArray *arr, int order_id);
void remove_by_id(OrderArray *arr, int order_id);