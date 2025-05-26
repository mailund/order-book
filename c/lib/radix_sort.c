#include "radix_sort.h"
#include "order.h"
#include <assert.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define MAX_BUCKETS 65536
#define PRICE_SHIFT 10000
#define PASSES_PRICE 1
#define PASSES_QUANTITY 2

// ---------- Extract Buckets ----------

static inline uint32_t bucket_quantity(Order *o, int radix) {
  return ((uint32_t)o->quantity >> (radix * 16)) & 0xFFFF;
}

static inline uint32_t bucket_price(Order *o, int radix) {
  return ((uint32_t)(o->price + PRICE_SHIFT) >> (radix * 16)) & 0xFFFF;
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

static void counting_sort(Order *const *src, Order **dst, size_t size,
                          uint32_t (*get_bucket)(Order *, int), int radix) {
  size_t count[MAX_BUCKETS] = {0};

  for (size_t i = 0; i < size; i++) {
    uint32_t b = get_bucket(src[i], radix);
    count[b]++;
  }

  for (int i = 1; i < MAX_BUCKETS; i++)
    count[i] += count[i - 1];

  for (ssize_t i = size - 1; i >= 0; i--) {
    uint32_t b = get_bucket(src[i], radix);
    dst[--count[b]] = src[i];
  }
}

// ---------- Public Interface ----------

void sort_asks_range(Order ***begin, Order **end) {
  size_t size = end - *begin;
  if (size == 0)
    return;

  Order **tmp = malloc(size * sizeof *tmp);
  if (!tmp) {
    perror("malloc");
    exit(1);
  }

  counting_sort(*begin, tmp, size, bucket_quantity, 0);
  counting_sort(tmp, *begin, size, bucket_quantity, 1);
  counting_sort(*begin, tmp, size, bucket_price, 0);
  memcpy(*begin, tmp, size * sizeof(Order *));

  free(tmp);
}

void sort_bids_range(Order ***begin, Order **end) {
  size_t size = end - *begin;
  if (size == 0)
    return;

  Order **tmp = malloc(size * sizeof *tmp);
  if (!tmp) {
    perror("malloc");
    exit(1);
  }

  counting_sort(*begin, tmp, size, bucket_quantity, 0);
  counting_sort(tmp, *begin, size, bucket_quantity, 1);
  counting_sort(*begin, tmp, size, bucket_price, 0);
  memcpy(*begin, tmp, size * sizeof(Order *));

  free(tmp);

  // Reverse for descending order
  reverse_range(*begin, size);
}