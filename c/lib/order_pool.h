// Allocation pools for non-moving Order structures

#pragma once

#include "order.h"
#include <stddef.h>

typedef struct OrderNode {
  Order order;
  struct OrderNode *next_free;
} OrderNode;

typedef struct OrderBlock {
  OrderNode *nodes;
  size_t capacity;
  size_t used;
  struct OrderBlock *next;
} OrderBlock;

typedef struct {
  OrderBlock *blocks;
  OrderNode *free_list;
  size_t block_capacity;
} OrderPool;

void init_order_pool(OrderPool *pool, size_t block_capacity);
void free_order_pool(OrderPool *pool);
Order *allocate_order(OrderPool *pool, int order_id, int order_type, int price,
                      int quantity);
