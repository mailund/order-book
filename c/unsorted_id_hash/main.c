#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "args.h"
#include "events.h"
#include "order.h"
#include "order_list_with_map.h"
#include "order_pool.h"

// ---------- Print Functions ----------

static void print_orders(const OrderArrayWithMap *orders) {
  for (size_t i = 0; i < orders->size; i++) {
    printf("\t");
    print_order(orders->data[i]);
  }
  printf("\n");
}

// ---------- Event Handlers ----------

static void handle_create(OrderArrayWithMap *buys, OrderArrayWithMap *sells,
                          const CreateOrder *co, int *order_id_counter,
                          OrderPool *pool) {
  Order *order = allocate_order(pool, (*order_id_counter)++,
                                co->side == SIDE_BUY ? ORDER_BUY : ORDER_SELL,
                                co->price, co->quantity);

  if (order->order_type == ORDER_BUY) {
    append_order_with_map(buys, order);
  } else {
    append_order_with_map(sells, order);
  }
}

static void handle_update(OrderArrayWithMap *buys, OrderArrayWithMap *sells,
                          const UpdateOrder *uo) {
  Order *order = find_order_by_id(buys, uo->order_id);
  if (!order)
    order = find_order_by_id(sells, uo->order_id);
  if (order)
    order->price = uo->price;
}

static void handle_remove(OrderArrayWithMap *buys, OrderArrayWithMap *sells,
                          int order_id) {
  remove_order_by_id(buys, order_id);
  remove_order_by_id(sells, order_id);
}

static void handle_bids(OrderArrayWithMap *buys, bool silent) {
  if (buys->size == 0)
    return;
  sort_orders_desc(buys);

  if (silent)
    return;

  printf("Bids\n");
  print_orders(buys);
}

static void handle_asks(OrderArrayWithMap *sells, bool silent) {
  if (sells->size == 0)
    return;
  sort_orders_asc(sells);

  if (silent)
    return;

  printf("Asks\n");
  print_orders(sells);
}

// ---------- Main ----------

int main(int argc, char *argv[]) {
  Config cfg;
  parse_args(&cfg, argc, argv);
  if (cfg.input_file) {
    freopen(cfg.input_file, "r", stdin);
  }

  OrderArrayWithMap buys, sells;
  init_order_array_with_map(&buys);
  init_order_array_with_map(&sells);

  OrderPool pool;
  init_order_pool(&pool, 1024); // Preallocate blocks of 1024 orders

  EventIterator iter;
  event_iterator_init(&iter, stdin);

  int order_id_counter = 0;
  Event event;

  while (event_iterator_next(&iter, &event)) {
    switch (event.type) {
    case EVENT_CREATE:
      handle_create(&buys, &sells, &event.data.create, &order_id_counter,
                    &pool);
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
  free_order_array_with_map(&buys);
  free_order_array_with_map(&sells);
  free_order_pool(&pool);

  return 0;
}