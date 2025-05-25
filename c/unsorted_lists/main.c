#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "events.h"
#include "order.h"
#include "order_array.h"

// Sort helpers

static int cmp_order_asc(const void *a, const void *b) {
  const Order *o1 = (const Order *)a;
  const Order *o2 = (const Order *)b;
  if (o1->price != o2->price)
    return o1->price - o2->price;
  return o1->quantity - o2->quantity;
}

static int cmp_order_desc(const void *a, const void *b) {
  const Order *o1 = (const Order *)a;
  const Order *o2 = (const Order *)b;
  if (o1->price != o2->price)
    return o2->price - o1->price;
  return o2->quantity - o1->quantity;
}

static void sort_orders_ascending(OrderArray *orders) {
  qsort(orders->data, orders->size, sizeof(Order), cmp_order_asc);
}

static void sort_orders_descending(OrderArray *orders) {
  qsort(orders->data, orders->size, sizeof(Order), cmp_order_desc);
}

// Print and creation

static void print_orders(const OrderArray *orders) {
  for (size_t i = 0; i < orders->size; i++) {
    printf("\t");
    print_order(order_at_index(orders, i));
  }
}

static void create_order(OrderArray *orders, int order_id,
                         const CreateOrder *co) {
  Order order =
      make_order(order_id, co->side == SIDE_BUY ? ORDER_BUY : ORDER_SELL,
                 co->price, co->quantity);
  append_order(orders, order);
}

// Event handling

static void handle_create(OrderArray *buys, OrderArray *sells,
                          const CreateOrder *co, int *order_id) {
  if (co->side == SIDE_BUY) {
    create_order(buys, (*order_id)++, co);
  } else {
    create_order(sells, (*order_id)++, co);
  }
}

static void handle_update(OrderArray *buys, OrderArray *sells,
                          const UpdateOrder *uo) {
  Order *order = order_by_id(buys, uo->order_id);
  if (!order)
    order = order_by_id(sells, uo->order_id);
  if (order)
    order->price = uo->price;
}

static void handle_remove(OrderArray *buys, OrderArray *sells, int order_id) {
  remove_by_id(buys, order_id);
  remove_by_id(sells, order_id);
}

static void handle_bids(OrderArray *buys, bool silent) {
  if (buys->size == 0)
    return;
  sort_orders_descending(buys);
  if (!silent) {
    printf("Bids\n");
    print_orders(buys);
    printf("\n");
  }
}

static void handle_asks(OrderArray *sells, bool silent) {
  if (sells->size == 0)
    return;
  sort_orders_ascending(sells);
  if (!silent) {
    printf("Asks\n");
    print_orders(sells);
    printf("\n");
  }
}

// Main

int main(int argc, char *argv[]) {
  bool silent = (argc > 1) && (strcmp(argv[1], "--silent") == 0 ||
                               strcmp(argv[1], "-s") == 0);

  OrderArray buys, sells;
  init_order_array(&buys);
  init_order_array(&sells);

  EventIterator it;
  event_iterator_init(&it, stdin);

  int order_id_counter = 0;
  Event event;
  while (event_iterator_next(&it, &event)) {
    switch (event.type) {
    case EVENT_CREATE:
      handle_create(&buys, &sells, &event.data.create, &order_id_counter);
      break;
    case EVENT_UPDATE:
      handle_update(&buys, &sells, &event.data.update);
      break;
    case EVENT_REMOVE:
      handle_remove(&buys, &sells, event.data.remove.order_id);
      break;
    case EVENT_BIDS:
      handle_bids(&buys, silent);
      break;
    case EVENT_ASKS:
      handle_asks(&sells, silent);
      break;
    }
  }

  event_iterator_close(&it);
  free_order_array(&buys);
  free_order_array(&sells);
  return 0;
}