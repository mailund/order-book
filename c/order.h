#pragma once

#include <stdio.h>

typedef enum { ORDER_BUY, ORDER_SELL } OrderType;

typedef struct {
  int order_id;
  OrderType order_type;
  int price;
  int quantity;
} Order;

inline Order make_order(int order_id, OrderType order_type, int price,
                        int quantity) {
  return (Order){.order_id = order_id,
                 .order_type = order_type,
                 .price = price,
                 .quantity = quantity};
}

void print_order(const Order *order);
void print_order_full(const Order *order);