#include "order.h"
#include <stdio.h>

static const char *order_type_to_str(OrderType type) {
  return (type == ORDER_BUY) ? "Buy" : "Sell";
}

void print_order(const Order *order) {
  printf("%s %d %d\n", order_type_to_str(order->order_type), order->price,
         order->quantity);
}
