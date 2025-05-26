#include "order_pool.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

void init_order_pool(OrderPool *pool, size_t block_capacity) {
  pool->blocks = NULL;
  pool->free_list = NULL;
  pool->block_capacity = block_capacity;
}

void free_order_pool(OrderPool *pool) {
  OrderBlock *block = pool->blocks;
  while (block) {
    OrderBlock *next = block->next;
    free(block->nodes);
    free(block);
    block = next;
  }
  pool->blocks = NULL;
  pool->free_list = NULL;
}

static OrderBlock *allocate_block(size_t capacity) {
  OrderBlock *block = malloc(sizeof *block);
  if (!block)
    return NULL;

  block->nodes = calloc(capacity, sizeof *block->nodes);
  if (!block->nodes) {
    free(block);
    return NULL;
  }

  block->capacity = capacity;
  block->used = 0;
  block->next = NULL;
  return block;
}

Order *allocate_order(OrderPool *pool, int order_id, int order_type, int price,
                      int quantity) {
  OrderNode *node = NULL;

  if (pool->free_list) {
    // Reuse node from free list
    node = pool->free_list;
    pool->free_list = node->next_free;

  } else {

    // Allocate new node from block
    if (!pool->blocks || pool->blocks->used == pool->blocks->capacity) {
      // Need a new block
      OrderBlock *new_block = allocate_block(pool->block_capacity);
      if (!new_block) {
        perror("Failed to allocate OrderBlock");
        exit(EXIT_FAILURE);
      }
      new_block->next = pool->blocks;
      pool->blocks = new_block;
    }
    node = &pool->blocks->nodes[pool->blocks->used++];
  }

  node->order = (Order){order_id, order_type, price, quantity};
  return &node->order;
}
