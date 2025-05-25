#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "events.h"
#include "order.h"
#include "order_array.h"
#include "radix_sort.h"

// Printing

static void print_orders(const OrderArray *orders) {
  for (size_t i = 0; i < orders->size; i++) {
    printf("\t");
    print_order(order_at_index(orders, i));
  }
  printf("\n");
}

// Event Handlers

static void handle_create(OrderArray *buys, OrderArray *sells, int *next_id,
                          const CreateOrder *create) {
  Order order = make_order((*next_id)++,
                           create->side == SIDE_BUY ? ORDER_BUY : ORDER_SELL,
                           create->price, create->quantity);

  if (order.order_type == ORDER_BUY) {
    append_order(buys, order);
  } else {
    append_order(sells, order);
  }
}

static void handle_update(OrderArray *buys, OrderArray *sells,
                          const UpdateOrder *update) {
  Order *order = order_by_id(buys, update->order_id);
  if (!order) {
    order = order_by_id(sells, update->order_id);
  }
  if (order) {
    order->price = update->price;
  }
}

static void handle_remove(OrderArray *buys, OrderArray *sells, int order_id) {
  remove_by_id(buys, order_id);
  remove_by_id(sells, order_id);
}

static void handle_bids(OrderArray *buys) {
  if (buys->size == 0)
    return;
  sort_bids(buys);
  printf("Bids\n");
  print_orders(buys);
}

static void handle_asks(OrderArray *sells) {
  if (sells->size == 0)
    return;
  sort_asks(sells);
  printf("Asks\n");
  print_orders(sells);
}

// Main

int main(void) {
  OrderArray buys, sells;
  init_order_array(&buys);
  init_order_array(&sells);

  EventIterator iter;
  event_iterator_init(&iter, stdin);

  int order_id_counter = 0;
  Event event;

  while (event_iterator_next(&iter, &event)) {
    switch (event.type) {
    case EVENT_CREATE:
      handle_create(&buys, &sells, &order_id_counter, &event.data.create);
      break;
    case EVENT_UPDATE:
      handle_update(&buys, &sells, &event.data.update);
      break;
    case EVENT_REMOVE:
      handle_remove(&buys, &sells, event.data.remove.order_id);
      break;
    case EVENT_BIDS:
      handle_bids(&buys);
      break;
    case EVENT_ASKS:
      handle_asks(&sells);
      break;
    }
  }

  event_iterator_close(&iter);
  free_order_array(&buys);
  free_order_array(&sells);
  return 0;
}