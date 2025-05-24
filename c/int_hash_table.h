#pragma once

#include <stdbool.h>
#include <stddef.h>

typedef struct {
  int key;
  int value;
} Entry;

typedef struct {
  Entry *entries;
  size_t capacity;
  size_t size;
} HashTable;

void hash_table_init(HashTable *table, size_t initial_capacity);
void hash_table_free(HashTable *table);
bool hash_table_set(HashTable *table, int key, int value);
bool hash_table_get(HashTable *table, int key, int *out_value);
bool hash_table_remove(HashTable *table, int key);
