// radix_sort_byte.c
#include <assert.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "radix_sort_byte.h"

#define MAX_BUCKETS 256
#define PRICE_SHIFT 10000
#define PASSES_PRICE 2
#define PASSES_QUANTITY 4

// ---------- Extract Buckets ----------

static inline uint32_t bucket_quantity(Order *o, int radix) {
  return ((uint32_t)o->quantity >> (radix * 8)) & 0xFF;
}

static inline uint32_t bucket_price(Order *o, int radix) {
  return ((uint32_t)(o->price + PRICE_SHIFT) >> (radix * 8)) & 0xFF;
}

// ---------- Reverse Utility ----------

static void reverse_range(Order **data, size_t size) {
  for (size_t i = 0; i < size / 2; i++) {
    Order *tmp = data[i];
    data[i] = data[size - 1 - i];
    data[size - 1 - i] = tmp;
  }
}

// ---------- Counting Sort (always ascending) ----------

static inline void counting_sort(Order *const *src, Order **dst, size_t size,
                                 uint32_t (*get_bucket)(Order *, int),
                                 int radix) {
  size_t count[MAX_BUCKETS] = {0};

  for (size_t i = 0; i < size; i++) {
    uint32_t b = get_bucket(src[i], radix);
    count[b]++;
  }

  for (size_t i = 1; i < MAX_BUCKETS; i++)
    count[i] += count[i - 1];

  for (ssize_t i = size - 1; i >= 0; i--) {
    uint32_t b = get_bucket(src[i], radix);
    dst[--count[b]] = src[i];
  }
}

// ---------- Public Interface ----------

void sort_asks_range_bytes(Order ***begin, Order **end) {
  size_t size = end - *begin;
  if (size == 0)
    return;

  Order **tmp = malloc(size * sizeof *tmp);
  if (!tmp) {
    perror("malloc");
    exit(1);
  }

  for (int i = 0; i < PASSES_QUANTITY; i++) {
    counting_sort(*begin, tmp, size, bucket_quantity, i);
    Order **swap = *begin;
    *begin = tmp;
    tmp = swap;
  }

  for (int i = 0; i < PASSES_PRICE; i++) {
    counting_sort(*begin, tmp, size, bucket_price, i);
    Order **swap = *begin;
    *begin = tmp;
    tmp = swap;
  }

  free(tmp);
}

void sort_bids_range_bytes(Order ***begin, Order **end) {
  size_t size = end - *begin;
  if (size == 0)
    return;

  Order **tmp = malloc(size * sizeof *tmp);
  if (!tmp) {
    perror("malloc");
    exit(1);
  }

  for (int i = 0; i < PASSES_QUANTITY; i++) {
    counting_sort(*begin, tmp, size, bucket_quantity, i);
    Order **swap = *begin;
    *begin = tmp;
    tmp = swap;
  }

  for (int i = 0; i < PASSES_PRICE; i++) {
    counting_sort(*begin, tmp, size, bucket_price, i);
    Order **swap = *begin;
    *begin = tmp;
    tmp = swap;
  }

  reverse_range(*begin, size);
  free(tmp);
}
