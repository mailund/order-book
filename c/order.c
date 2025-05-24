#include "order.h"
#include <stdio.h>

static const char *order_type_to_str(OrderType type) {
  return (type == ORDER_BUY) ? "Buy" : "Sell";
}

void print_order(const Order *order) {
  printf("%s %d %d\n", order_type_to_str(order->order_type), order->price,
         order->quantity);
}
void print_order_full(const Order *order) {
  printf("Order ID: %d, Type: %s, Price: %d, Quantity: %d\n", order->order_id,
         order->order_type == ORDER_BUY ? "BUY" : "SELL", order->price,
         order->quantity);
}
