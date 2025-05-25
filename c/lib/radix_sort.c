#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "radix_sort.h"

#define MAX_DIGIT 256
#define PASSES_PRICE 2    // 2 bytes for range [-10_000, 10_000]
#define PASSES_QUANTITY 4 // Full 32-bit range
#define PRICE_SHIFT 10000 // Shift prices into non-negative range

// ----------------- COUNTING SORT HELPERS -----------------

static void counting_sort_price_ptr_asc(Order **src, Order **dst, size_t size,
                                        int byte) {
  int count[MAX_DIGIT] = {0};

  for (size_t i = 0; i < size; i++) {
    uint32_t key = (uint32_t)(src[i]->price + PRICE_SHIFT);
    int b = (key >> (byte * 8)) & 0xFF;
    count[b]++;
  }
  for (int i = 1; i < MAX_DIGIT; i++)
    count[i] += count[i - 1];

  for (ssize_t i = size - 1; i >= 0; i--) {
    uint32_t key = (uint32_t)(src[i]->price + PRICE_SHIFT);
    int b = (key >> (byte * 8)) & 0xFF;
    dst[--count[b]] = src[i];
  }
}

static void counting_sort_price_ptr_desc(Order **src, Order **dst, size_t size,
                                         int byte) {
  int count[MAX_DIGIT] = {0};

  for (size_t i = 0; i < size; i++) {
    uint32_t key = (uint32_t)(src[i]->price + PRICE_SHIFT);
    int b = (key >> (byte * 8)) & 0xFF;
    count[b]++;
  }
  for (int i = MAX_DIGIT - 2; i >= 0; i--)
    count[i] += count[i + 1];

  for (ssize_t i = size - 1; i >= 0; i--) {
    uint32_t key = (uint32_t)(src[i]->price + PRICE_SHIFT);
    int b = (key >> (byte * 8)) & 0xFF;
    dst[--count[b]] = src[i];
  }
}

static void counting_sort_quantity_ptr_asc(Order **src, Order **dst,
                                           size_t size, int byte) {
  int count[MAX_DIGIT] = {0};

  for (size_t i = 0; i < size; i++) {
    uint32_t key = (uint32_t)src[i]->quantity;
    int b = (key >> (byte * 8)) & 0xFF;
    count[b]++;
  }
  for (int i = 1; i < MAX_DIGIT; i++)
    count[i] += count[i - 1];

  for (ssize_t i = size - 1; i >= 0; i--) {
    uint32_t key = (uint32_t)src[i]->quantity;
    int b = (key >> (byte * 8)) & 0xFF;
    dst[--count[b]] = src[i];
  }
}

static void counting_sort_quantity_ptr_desc(Order **src, Order **dst,
                                            size_t size, int byte) {
  int count[MAX_DIGIT] = {0};

  for (size_t i = 0; i < size; i++) {
    uint32_t key = (uint32_t)src[i]->quantity;
    int b = (key >> (byte * 8)) & 0xFF;
    count[b]++;
  }
  for (int i = MAX_DIGIT - 2; i >= 0; i--)
    count[i] += count[i + 1];

  for (ssize_t i = size - 1; i >= 0; i--) {
    uint32_t key = (uint32_t)src[i]->quantity;
    int b = (key >> (byte * 8)) & 0xFF;
    dst[--count[b]] = src[i];
  }
}

// ----------------- RADIX SORT WRAPPER -----------------

static void radix_sort_ptr(Order ***ptrs, size_t size,
                           void (*counting_sort)(Order **, Order **, size_t,
                                                 int),
                           int passes, Order ***buf) {
  Order **src = *ptrs;
  Order **dst = *buf;

  for (int byte = 0; byte < passes; byte++) {
    counting_sort(src, dst, size, byte);
    Order **tmp = src;
    src = dst;
    dst = tmp;
  }

  if (src != *ptrs) {
    *ptrs = src;
    *buf = dst;
  }
}

// ----------------- PUBLIC SORT INTERFACES -----------------

void sort_asks(OrderArray *arr) {
  if (!arr || arr->size == 0)
    return;

  Order **ptrs = malloc(arr->size * sizeof(Order *));
  Order **buf = malloc(arr->size * sizeof(Order *));
  if (!ptrs || !buf) {
    perror("malloc ptrs/buf");
    exit(1);
  }

  for (size_t i = 0; i < arr->size; i++) {
    ptrs[i] = &arr->data[i];
  }

  radix_sort_ptr(&ptrs, arr->size, counting_sort_quantity_ptr_asc,
                 PASSES_QUANTITY, &buf);
  radix_sort_ptr(&ptrs, arr->size, counting_sort_price_ptr_asc, PASSES_PRICE,
                 &buf);

  Order *copy = malloc(arr->size * sizeof(Order));
  if (!copy) {
    perror("malloc copy");
    exit(1);
  }

  // Make a copy of the Orders so we can rearrange them without overwriting
  // the original data.
  for (size_t i = 0; i < arr->size; i++) {
    copy[i] = *ptrs[i];
  }
  memcpy(arr->data, copy, arr->size * sizeof(Order));

  free(copy);
  free(ptrs);
  free(buf);
}

void sort_bids(OrderArray *arr) {
  if (!arr || arr->size == 0)
    return;

  Order **ptrs = malloc(arr->size * sizeof(Order *));
  Order **buf = malloc(arr->size * sizeof(Order *));
  if (!ptrs || !buf) {
    perror("malloc ptrs/buf");
    exit(1);
  }

  for (size_t i = 0; i < arr->size; i++) {
    ptrs[i] = &arr->data[i];
  }

  radix_sort_ptr(&ptrs, arr->size, counting_sort_quantity_ptr_desc,
                 PASSES_QUANTITY, &buf);
  radix_sort_ptr(&ptrs, arr->size, counting_sort_price_ptr_desc, PASSES_PRICE,
                 &buf);

  Order *copy = malloc(arr->size * sizeof(Order));
  if (!copy) {
    perror("malloc copy");
    exit(1);
  }

  for (size_t i = 0; i < arr->size; i++) {
    copy[i] = *ptrs[i];
  }

  memcpy(arr->data, copy, arr->size * sizeof(Order));

  free(copy);
  free(ptrs);
  free(buf);
}