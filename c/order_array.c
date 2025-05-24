#include "order_array.h"

#include <stdio.h>

void init_order_array(OrderArray *arr) {
  arr->size = 0;
  arr->capacity = 4;
  arr->data = malloc(arr->capacity * sizeof(Order));
  if (!arr->data) {
    perror("malloc");
    exit(1);
  }
}

void free_order_array(OrderArray *arr) {
  free(arr->data);
  arr->data = NULL;
  arr->size = arr->capacity = 0;
}

void append_order(OrderArray *arr, Order order) {
  if (arr->size == arr->capacity) {
    arr->capacity *= 2;
    Order *new_data = realloc(arr->data, arr->capacity * sizeof(Order));
    if (!new_data) {
      perror("realloc");
      exit(1);
    }
    arr->data = new_data;
  }
  arr->data[arr->size++] = order;
}

Order *order_at_index(OrderArray *arr, size_t index) {
  if (index >= arr->size) {
    fprintf(stderr, "Index out of bounds\n");
    exit(1);
  }
  return &arr->data[index];
}

Order *order_by_id(OrderArray *arr, int order_id) {
  for (size_t i = 0; i < arr->size; i++) {
    if (arr->data[i].order_id == order_id) {
      return &arr->data[i];
    }
  }
  return NULL; // Not found
}

static inline void swap(Order *a, Order *b) {
  Order tmp = *a;
  *a = *b;
  *b = tmp;
}

void remove_by_index(OrderArray *arr, size_t index) {
  Order *order = &arr->data[index];
  Order *last_order = &arr->data[arr->size - 1];
  if (order != last_order) {
    swap(order, last_order);
  }

  arr->size--;
}

void remove_by_id(OrderArray *arr, int order_id) {
  Order *order = order_by_id(arr, order_id);
  if (order) {
    size_t index = order - arr->data; // Calculate index from pointer
    remove_by_index(arr, index);
  }
}
