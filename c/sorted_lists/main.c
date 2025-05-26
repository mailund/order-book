#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "args.h"
#include "events.h"
#include "order.h"
#include "order_array.h"

static int cmp_order_asc(const Order *o1, const Order *o2) {
  if (o1->price != o2->price)
    return o1->price - o2->price;
  return o1->quantity - o2->quantity;
}

static int cmp_order_desc(const Order *o1, const Order *o2) {
  if (o1->price != o2->price)
    return o2->price - o1->price;
  return o2->quantity - o1->quantity;
}

typedef struct {
  OrderArray *orders;
  int (*cmp)(const Order *, const Order *);
} SortedOrders;

static void init_sorted_orders(SortedOrders *orders,
                               int (*cmp)(const Order *, const Order *)) {
  orders->orders = malloc(sizeof(OrderArray));
  init_order_array(orders->orders);
  orders->cmp = cmp;
}

static void insert_sorted(SortedOrders *orders, Order order) {
  // Append the order and then bubble it down to its correct position
  OrderArray *arr = orders->orders;
  append_order(arr, order);

  size_t i = arr->size - 1;
  while (i > 0 && orders->cmp(&arr->data[i], &arr->data[i - 1]) < 0) {
    Order temp = arr->data[i];
    arr->data[i] = arr->data[i - 1];
    arr->data[i - 1] = temp;
    i--;
  }
}

// Event handling

static void handle_create(SortedOrders *buys, SortedOrders *sells,
                          const CreateOrder *co, int *order_id) {
  Order order =
      make_order((*order_id)++, co->side == SIDE_BUY ? ORDER_BUY : ORDER_SELL,
                 co->price, co->quantity);
  if (co->side == SIDE_BUY) {
    insert_sorted(buys, order);
  } else {
    insert_sorted(sells, order);
  }
}

static void reorder_order(SortedOrders *orders, size_t index) {
  OrderArray *arr = orders->orders;
  // Bubble upward if needed
  while (index > 0 &&
         orders->cmp(&arr->data[index], &arr->data[index - 1]) < 0) {
    Order temp = arr->data[index];
    arr->data[index] = arr->data[index - 1];
    arr->data[index - 1] = temp;
    index--;
  }
  // Bubble downward if needed
  while (index < arr->size - 1 &&
         orders->cmp(&arr->data[index], &arr->data[index + 1]) > 0) {
    Order temp = arr->data[index];
    arr->data[index] = arr->data[index + 1];
    arr->data[index + 1] = temp;
    index++;
  }
}

static void handle_update(SortedOrders *buys, SortedOrders *sells,
                          const UpdateOrder *uo) {
  Order *order = NULL;
  if ((order = order_by_id(buys->orders, uo->order_id))) {
    order->price = uo->price;
    reorder_order(buys, order - buys->orders->data);
  }
  if ((order = order_by_id(sells->orders, uo->order_id))) {
    order->price = uo->price;
    reorder_order(sells, order - sells->orders->data);
  }
}

static void remove_id(OrderArray *orders, int order_id) {
  for (size_t i = 0; i < orders->size; i++) {
    if (orders->data[i].order_id == order_id) {
      // Shift later elements one position to the left
      for (size_t j = i; j < orders->size - 1; j++) {
        orders->data[j] = orders->data[j + 1];
      }
      orders->size--;
      break;
    }
  }
}

static void handle_remove(SortedOrders *buys, SortedOrders *sells,
                          int order_id) {
  remove_id(buys->orders, order_id);
  remove_id(sells->orders, order_id);
}

static void print_orders(const OrderArray *orders) {
  for (size_t i = 0; i < orders->size; i++) {
    printf("\t");
    print_order(order_at_index(orders, i));
  }
}

static void handle_bids(SortedOrders *buys, bool silent) {
  if (buys->orders->size == 0)
    return;

  if (!silent) {
    printf("Bids\n");
    print_orders(buys->orders);
    printf("\n");
  }
}

static void handle_asks(SortedOrders *sells, bool silent) {
  if (sells->orders->size == 0)
    return;

  if (!silent) {
    printf("Asks\n");
    print_orders(sells->orders);
    printf("\n");
  }
}

// Main

int main(int argc, char *argv[]) {
  Config cfg;
  parse_args(&cfg, argc, argv);
  if (cfg.input_file) {
    freopen(cfg.input_file, "r", stdin);
  }

  SortedOrders buys, sells;
  init_sorted_orders(&buys, cmp_order_desc);
  init_sorted_orders(&sells, cmp_order_asc);

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
      handle_bids(&buys, cfg.silent);
      break;
    case EVENT_ASKS:
      handle_asks(&sells, cfg.silent);
      break;
    }
  }

  event_iterator_close(&it);
  free_order_array(buys.orders);
  free_order_array(sells.orders);

  return 0;
}