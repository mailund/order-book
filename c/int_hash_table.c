#include "int_hash_table.h"
#include <stdlib.h>
#include <string.h>

#define EMPTY_KEY -1

static inline size_t next_power_of_two(size_t x) {
  size_t power = 1;
  while (power < x)
    power <<= 1;
  return power;
}

static inline size_t hash_bin(int key, size_t capacity) {
  return (unsigned int)key & (capacity - 1); // fast for power-of-two capacity
}

void hash_table_init(HashTable *table, size_t initial_capacity) {
  table->capacity = next_power_of_two(initial_capacity);
  table->size = 0;
  table->entries = calloc(table->capacity, sizeof(Entry));
  for (size_t i = 0; i < table->capacity; ++i)
    table->entries[i].key = EMPTY_KEY;
}

void hash_table_free(HashTable *table) {
  free(table->entries);
  table->entries = NULL;
  table->capacity = table->size = 0;
}

static void hash_table_resize(HashTable *table) {
  size_t new_capacity = table->capacity * 2;
  Entry *old_entries = table->entries;
  size_t old_capacity = table->capacity;

  table->entries = calloc(new_capacity, sizeof(Entry));
  table->capacity = new_capacity;
  table->size = 0;

  for (size_t i = 0; i < new_capacity; ++i)
    table->entries[i].key = EMPTY_KEY;

  for (size_t i = 0; i < old_capacity; ++i) {
    if (old_entries[i].key != EMPTY_KEY) {
      hash_table_set(table, old_entries[i].key, old_entries[i].value);
    }
  }

  free(old_entries);
}

bool hash_table_set(HashTable *table, int key, int value) {
  if (table->size * 2 >= table->capacity)
    hash_table_resize(table);

  size_t idx = hash_bin(key, table->capacity);
  while (table->entries[idx].key != EMPTY_KEY &&
         table->entries[idx].key != key) {
    idx = (idx + 1) & (table->capacity - 1);
  }

  if (table->entries[idx].key == EMPTY_KEY)
    table->size++;
  table->entries[idx].key = key;
  table->entries[idx].value = value;
  return true;
}

bool hash_table_get(HashTable *table, int key, int *out_value) {
  size_t idx = hash_bin(key, table->capacity);
  size_t start = idx;

  while (table->entries[idx].key != EMPTY_KEY) {
    if (table->entries[idx].key == key) {
      *out_value = table->entries[idx].value;
      return true;
    }
    idx = (idx + 1) & (table->capacity - 1);
    if (idx == start)
      break; // full cycle
  }

  return false;
}

bool hash_table_remove(HashTable *table, int key) {
  size_t idx = hash_bin(key, table->capacity);
  while (table->entries[idx].key != EMPTY_KEY) {
    if (table->entries[idx].key == key) {
      table->entries[idx].key = EMPTY_KEY;
      table->size--;
      return true;
    }
    idx = (idx + 1) & (table->capacity - 1);
  }
  return false;
}
