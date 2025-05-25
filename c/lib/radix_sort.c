#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "radix_sort.h"

#define MAX_DIGIT 256
#define PASSES_PRICE 2    // Only 2 bytes needed for range [0, 20000]
#define PASSES_QUANTITY 4 // Assume full 32-bit range
#define PRICE_SHIFT 10000 // Shifts [-10000,10000] into [0,20000]

// ---------- PRICE ASCENDING ----------

static void counting_sort_price_asc(Order *src, Order *dst, size_t size,
                                    int byte) {
  int count[MAX_DIGIT] = {0};

  for (size_t i = 0; i < size; i++) {
    uint32_t key = (uint32_t)(src[i].price + PRICE_SHIFT);
    int b = (key >> (byte * 8)) & 0xFF;
    count[b]++;
  }
  for (int i = 1; i < MAX_DIGIT; i++) {
    count[i] += count[i - 1];
  }

  for (ssize_t i = size - 1; i >= 0; i--) {
    uint32_t key = (uint32_t)(src[i].price + PRICE_SHIFT);
    int b = (key >> (byte * 8)) & 0xFF;
    dst[--count[b]] = src[i];
  }
}

void radix_sort_by_price_asc(Order **data, size_t size, Order **tmp) {
  Order *src = *data;
  Order *dst = *tmp;

  for (int byte = 0; byte < PASSES_PRICE; byte++) {
    counting_sort_price_asc(src, dst, size, byte);
    Order *swap = src;
    src = dst;
    dst = swap;
  }

  *data = src;
  *tmp = dst;
}

// ---------- PRICE DESCENDING ----------

static void counting_sort_price_desc(Order *src, Order *dst, size_t size,
                                     int byte) {
  int count[MAX_DIGIT] = {0};

  for (size_t i = 0; i < size; i++) {
    uint32_t key = (uint32_t)(src[i].price + PRICE_SHIFT);
    int b = (key >> (byte * 8)) & 0xFF;
    count[b]++;
  }
  for (int i = MAX_DIGIT - 2; i >= 0; i--) {
    count[i] += count[i + 1];
  }

  for (ssize_t i = size - 1; i >= 0; i--) {
    uint32_t key = (uint32_t)(src[i].price + PRICE_SHIFT);
    int b = (key >> (byte * 8)) & 0xFF;
    dst[--count[b]] = src[i];
  }
}

void radix_sort_by_price_desc(Order **data, size_t size, Order **tmp) {
  Order *src = *data;
  Order *dst = *tmp;

  for (int byte = 0; byte < PASSES_PRICE; byte++) {
    counting_sort_price_desc(src, dst, size, byte);
    Order *swap = src;
    src = dst;
    dst = swap;
  }

  *data = src;
  *tmp = dst;
}

// ---------- QUANTITY ASCENDING ----------

static void counting_sort_quantity_asc(Order *src, Order *dst, size_t size,
                                       int byte) {
  int count[MAX_DIGIT] = {0};

  for (size_t i = 0; i < size; i++) {
    uint32_t key = (uint32_t)src[i].quantity;
    int b = (key >> (byte * 8)) & 0xFF;
    count[b]++;
  }
  for (int i = 1; i < MAX_DIGIT; i++) {
    count[i] += count[i - 1];
  }

  for (ssize_t i = size - 1; i >= 0; i--) {
    uint32_t key = (uint32_t)src[i].quantity;
    int b = (key >> (byte * 8)) & 0xFF;
    dst[--count[b]] = src[i];
  }
}

void radix_sort_by_quantity_asc(Order **data, size_t size, Order **tmp) {
  Order *src = *data;
  Order *dst = *tmp;

  for (int byte = 0; byte < PASSES_QUANTITY; byte++) {
    counting_sort_quantity_asc(src, dst, size, byte);
    Order *swap = src;
    src = dst;
    dst = swap;
  }

  *data = src;
  *tmp = dst;
}

// ---------- QUANTITY DESCENDING ----------

static void counting_sort_quantity_desc(Order *src, Order *dst, size_t size,
                                        int byte) {
  int count[MAX_DIGIT] = {0};

  for (size_t i = 0; i < size; i++) {
    uint32_t key = (uint32_t)src[i].quantity;
    int b = (key >> (byte * 8)) & 0xFF;
    count[b]++;
  }
  for (int i = MAX_DIGIT - 2; i >= 0; i--) {
    count[i] += count[i + 1];
  }

  for (ssize_t i = size - 1; i >= 0; i--) {
    uint32_t key = (uint32_t)src[i].quantity;
    int b = (key >> (byte * 8)) & 0xFF;
    dst[--count[b]] = src[i];
  }
}

void radix_sort_by_quantity_desc(Order **data, size_t size, Order **tmp) {
  Order *src = *data;
  Order *dst = *tmp;

  for (int byte = 0; byte < PASSES_QUANTITY; byte++) {
    counting_sort_quantity_desc(src, dst, size, byte);
    Order *swap = src;
    src = dst;
    dst = swap;
  }

  *data = src;
  *tmp = dst;
}

void sort_asks(OrderArray *arr) {
  if (!arr || arr->size == 0)
    return;

  Order *tmp = malloc(arr->size * sizeof(Order));
  if (!tmp) {
    perror("malloc");
    exit(EXIT_FAILURE);
  }

  Order *data = arr->data;
  Order *tmp_buf = tmp;

  // Sort by quantity (secondary key)
  radix_sort_by_quantity_asc(&data, arr->size, &tmp_buf);
  // Sort by price (primary key)
  radix_sort_by_price_asc(&data, arr->size, &tmp_buf);

  if (data != arr->data) {
    memcpy(arr->data, data, arr->size * sizeof(Order));
  }

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

  Order *data = arr->data;
  Order *tmp_buf = tmp;

  // Sort by quantity (secondary key)
  radix_sort_by_quantity_desc(&data, arr->size, &tmp_buf);
  // Sort by price (primary key)
  radix_sort_by_price_desc(&data, arr->size, &tmp_buf);

  if (data != arr->data) {
    memcpy(arr->data, data, arr->size * sizeof(Order));
  }

  free(tmp);
}
