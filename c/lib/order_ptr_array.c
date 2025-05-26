#include "order_ptr_array.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define INITIAL_CAPACITY 8

void init_order_ptr_array(OrderPtrArray *arr) {
  arr->size = 0;
  arr->capacity = INITIAL_CAPACITY;
  arr->data = malloc(arr->capacity * sizeof(Order *));
  if (!arr->data) {
    perror("malloc OrderPtrArray");
    exit(EXIT_FAILURE);
  }
}

void free_order_ptr_array(OrderPtrArray *arr) {
  free(arr->data);
  arr->data = NULL;
  arr->size = 0;
  arr->capacity = 0;
}

void append_order_ptr(OrderPtrArray *arr, Order *order) {
  if (arr->size == arr->capacity) {
    arr->capacity *= 2;
    arr->data = realloc(arr->data, arr->capacity * sizeof(Order *));
    if (!arr->data) {
      perror("realloc OrderPtrArray");
      exit(EXIT_FAILURE);
    }
  }
  arr->data[arr->size++] = order;
}

Order *order_ptr_at_index(const OrderPtrArray *arr, size_t index) {
  if (index >= arr->size)
    return NULL;
  return arr->data[index];
}

Order *order_ptr_by_id(const OrderPtrArray *arr, int order_id) {
  for (size_t i = 0; i < arr->size; i++) {
    if (arr->data[i]->order_id == order_id)
      return arr->data[i];
  }
  return NULL;
}

void remove_order_ptr_by_id(OrderPtrArray *arr, int order_id) {
  for (size_t i = 0; i < arr->size; i++) {
    if (arr->data[i]->order_id == order_id) {
      arr->data[i] = arr->data[--arr->size];
      return;
    }
  }
}
