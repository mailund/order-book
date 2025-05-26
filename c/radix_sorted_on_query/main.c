#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "args.h"
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
static void create_order(OrderArray *orders, int order_id,
                         const CreateOrder *co) {
  Order order =
      make_order(order_id, co->side == SIDE_BUY ? ORDER_BUY : ORDER_SELL,
                 co->price, co->quantity);
  append_order(orders, order);
}

static void handle_create(OrderArray *buys, OrderArray *sells,
                          const CreateOrder *co, int *order_id) {
  if (co->side == SIDE_BUY) {
    create_order(buys, (*order_id)++, co);
  } else {
    create_order(sells, (*order_id)++, co);
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

static void handle_bids(OrderArray *buys, bool silent) {
  if (buys->size == 0)
    return;
  sort_bids(buys);

  if (silent)
    return;
  printf("Bids\n");
  print_orders(buys);
}

static void handle_asks(OrderArray *sells, bool silent) {
  if (sells->size == 0)
    return;
  sort_asks(sells);

  if (silent)
    return;
  printf("Asks\n");
  print_orders(sells);
}

// Main

int main(int argc, char *argv[]) {
  Config cfg;
  parse_args(&cfg, argc, argv);
  if (cfg.input_file) {
    freopen(cfg.input_file, "r", stdin);
  }

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
      handle_create(&buys, &sells, &event.data.create, &order_id_counter);
      break;
    case EVENT_UPDATE:
      handle_update(&buys, &sells, &event.data.update);
      break;
    case EVENT_REMOVE:
      handle_remove(&buys, &sells, event.data.remove.order_id);
      break;
    case EVENT_BIDS:
      handle_bids(&buys, cfg.silent);
      break;
    case EVENT_ASKS:
      handle_asks(&sells, cfg.silent);
      break;
    }
  }

  event_iterator_close(&iter);
  free_order_array(&buys);
  free_order_array(&sells);
  return 0;
}