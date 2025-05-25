#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "events.h"
#include "order.h"
#include "order_array.h"
#include "order_map.h"

// Sorting Functions

static int cmp_order_asc(const void *a, const void *b) {
  const Order *o1 = a, *o2 = b;
  if (o1->price != o2->price)
    return o1->price - o2->price;
  return o1->quantity - o2->quantity;
}

static int cmp_order_desc(const void *a, const void *b) {
  const Order *o1 = a, *o2 = b;
  if (o1->price != o2->price)
    return o2->price - o1->price;
  return o2->quantity - o1->quantity;
}

static void sort_orders_ascending(OrderArray *orders, OrderMap *map) {
  qsort(orders->data, orders->size, sizeof(Order), cmp_order_asc);
  for (size_t i = 0; i < orders->size; i++) {
    order_map_set(map, orders->data[i].order_id, orders, &orders->data[i]);
  }
}

static void sort_orders_descending(OrderArray *orders, OrderMap *map) {
  qsort(orders->data, orders->size, sizeof(Order), cmp_order_desc);
  for (size_t i = 0; i < orders->size; i++) {
    order_map_set(map, orders->data[i].order_id, orders, &orders->data[i]);
  }
}

// Print Functions

static void print_orders(const OrderArray *orders) {
  for (size_t i = 0; i < orders->size; i++) {
    printf("\t");
    print_order(order_at_index(orders, i));
  }
  printf("\n");
}

// Event Handlers

static void handle_create(OrderArray *buys, OrderArray *sells, OrderMap *map,
                          const CreateOrder *co, int *order_id_counter) {
  Order order = make_order((*order_id_counter)++,
                           co->side == SIDE_BUY ? ORDER_BUY : ORDER_SELL,
                           co->price, co->quantity);

  if (order.order_type == ORDER_BUY) {
    append_order(buys, order);
    order_map_set(map, order.order_id, buys, &buys->data[buys->size - 1]);
  } else {
    append_order(sells, order);
    order_map_set(map, order.order_id, sells, &sells->data[sells->size - 1]);
  }
}

static void handle_update(OrderMap *map, const UpdateOrder *uo) {
  Order *order = entry_as_order(order_map_get(map, uo->order_id));
  if (order)
    order->price = uo->price;
}

static void handle_remove(OrderArray *buys, OrderArray *sells, OrderMap *map,
                          int order_id) {
  Order *order = entry_as_order(order_map_get(map, order_id));
  if (!order)
    return;

  if (points_into(buys, order)) {
    Order *last = &buys->data[buys->size - 1];
    order_map_set(map, last->order_id, buys, order);
    remove_by_index(buys, pointer_index(buys, order));
  } else if (points_into(sells, order)) {
    Order *last = &sells->data[sells->size - 1];
    order_map_set(map, last->order_id, sells, order);
    remove_by_index(sells, pointer_index(sells, order));
  }

  order_map_remove(map, order_id);
}

static void handle_bids(OrderArray *buys, OrderMap *map) {
  if (buys->size == 0)
    return;
  sort_orders_descending(buys, map);
  printf("Bids\n");
  print_orders(buys);
}

static void handle_asks(OrderArray *sells, OrderMap *map) {
  if (sells->size == 0)
    return;
  sort_orders_ascending(sells, map);
  printf("Asks\n");
  print_orders(sells);
}

// Main Function

int main(void) {
  OrderArray buys, sells;
  init_order_array(&buys);
  init_order_array(&sells);

  OrderMap map;
  order_map_init(&map, 16);

  EventIterator iter;
  event_iterator_init(&iter, stdin);

  int order_id_counter = 0;
  Event event;

  while (event_iterator_next(&iter, &event)) {
    switch (event.type) {
    case EVENT_CREATE:
      handle_create(&buys, &sells, &map, &event.data.create, &order_id_counter);
      break;

    case EVENT_UPDATE:
      handle_update(&map, &event.data.update);
      break;

    case EVENT_REMOVE:
      handle_remove(&buys, &sells, &map, event.data.remove.order_id);
      break;

    case EVENT_BIDS:
      handle_bids(&buys, &map);
      break;

    case EVENT_ASKS:
      handle_asks(&sells, &map);
      break;
    }
  }

  event_iterator_close(&iter);
  free_order_array(&buys);
  free_order_array(&sells);
  order_map_free(&map);
  return 0;
}