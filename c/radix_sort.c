#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "radix_sort.h"

// Radix sort by byte
#define MAX_DIGIT 256
#define PASSES 4

// For dealing with negative numbers
static inline uint32_t transform_signed(int x) {
  return (uint32_t)(x ^ 0x80000000);
}

//
// ASCENDING: by signed price
//
static void counting_sort_price_asc(Order *src, Order *dst, size_t size,
                                    int byte) {
  int count[MAX_DIGIT] = {0};

  for (size_t i = 0; i < size; i++) {
    uint32_t key = transform_signed(src[i].price);
    int b = (key >> (byte * 8)) & 0xFF;
    count[b]++;
  }
  for (int i = 1; i < MAX_DIGIT; i++)
    count[i] += count[i - 1];

  for (ssize_t i = size - 1; i >= 0; i--) {
    uint32_t key = transform_signed(src[i].price);
    int b = (key >> (byte * 8)) & 0xFF;
    dst[--count[b]] = src[i];
  }
}

static void radix_sort_by_price_asc(Order *data, size_t size, Order *tmp) {
  for (int byte = 0; byte < PASSES; byte++) {
    counting_sort_price_asc(data, tmp, size, byte);
    memcpy(data, tmp, size * sizeof(Order));
  }
}

//
// DESCENDING: by signed price
//
static void counting_sort_price_desc(Order *src, Order *dst, size_t size,
                                     int byte) {
  int count[MAX_DIGIT] = {0};

  for (size_t i = 0; i < size; i++) {
    uint32_t key = transform_signed(src[i].price);
    int b = (key >> (byte * 8)) & 0xFF;
    count[b]++;
  }
  for (int i = MAX_DIGIT - 2; i >= 0; i--)
    count[i] += count[i + 1];

  for (ssize_t i = size - 1; i >= 0; i--) {
    uint32_t key = transform_signed(src[i].price);
    int b = (key >> (byte * 8)) & 0xFF;
    dst[--count[b]] = src[i];
  }
}

static void radix_sort_by_price_desc(Order *data, size_t size, Order *tmp) {
  for (int byte = 0; byte < PASSES; byte++) {
    counting_sort_price_desc(data, tmp, size, byte);
    memcpy(data, tmp, size * sizeof(Order));
  }
}

//
// ASCENDING: by quantity (non-negative only)
//
static void counting_sort_quantity_asc(Order *src, Order *dst, size_t size,
                                       int byte) {
  int count[MAX_DIGIT] = {0};

  for (size_t i = 0; i < size; i++) {
    uint32_t key = (uint32_t)src[i].quantity;
    int b = (key >> (byte * 8)) & 0xFF;
    count[b]++;
  }
  for (int i = 1; i < MAX_DIGIT; i++)
    count[i] += count[i - 1];

  for (ssize_t i = size - 1; i >= 0; i--) {
    uint32_t key = (uint32_t)src[i].quantity;
    int b = (key >> (byte * 8)) & 0xFF;
    dst[--count[b]] = src[i];
  }
}

static void radix_sort_by_quantity_asc(Order *data, size_t size, Order *tmp) {
  for (int byte = 0; byte < PASSES; byte++) {
    counting_sort_quantity_asc(data, tmp, size, byte);
    memcpy(data, tmp, size * sizeof(Order));
  }
}

//
// DESCENDING: by quantity (non-negative only)
//
static void counting_sort_quantity_desc(Order *src, Order *dst, size_t size,
                                        int byte) {
  int count[MAX_DIGIT] = {0};

  for (size_t i = 0; i < size; i++) {
    uint32_t key = (uint32_t)src[i].quantity;
    int b = (key >> (byte * 8)) & 0xFF;
    count[b]++;
  }
  for (int i = MAX_DIGIT - 2; i >= 0; i--)
    count[i] += count[i + 1];

  for (ssize_t i = size - 1; i >= 0; i--) {
    uint32_t key = (uint32_t)src[i].quantity;
    int b = (key >> (byte * 8)) & 0xFF;
    dst[--count[b]] = src[i];
  }
}

static void radix_sort_by_quantity_desc(Order *data, size_t size, Order *tmp) {
  for (int byte = 0; byte < PASSES; byte++) {
    counting_sort_quantity_desc(data, tmp, size, byte);
    memcpy(data, tmp, size * sizeof(Order));
  }
}

void sort_asks(OrderArray *arr) {
  if (!arr || arr->size == 0)
    return;

  Order *tmp = malloc(arr->size * sizeof(Order));
  if (!tmp) {
    perror("malloc");
    exit(EXIT_FAILURE);
  }

  // Sort by quantity first (secondary key)
  radix_sort_by_quantity_asc(arr->data, arr->size, tmp);
  // Then by price (primary key)
  radix_sort_by_price_asc(arr->data, arr->size, tmp);

  free(tmp);
}

void sort_bids(OrderArray *arr) {
  if (!arr || arr->size == 0)
    return;

  Order *tmp = malloc(arr->size * sizeof(Order));
  if (!tmp) {
    perror("malloc");
    exit(EXIT_FAILURE);
  }

  // Sort by quantity first (secondary key)
  radix_sort_by_quantity_desc(arr->data, arr->size, tmp);
  // Then by price (primary key)
  radix_sort_by_price_desc(arr->data, arr->size, tmp);

  free(tmp);
}