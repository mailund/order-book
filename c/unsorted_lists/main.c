#include <stdio.h>

#include "events.h"
#include "order.h"
#include "order_array.h"

#include <stdlib.h>

static int cmp_order_asc(const void *a, const void *b) {
  const Order *o1 = (const Order *)a;
  const Order *o2 = (const Order *)b;
  if (o1->price < o2->price)
    return -1;
  if (o1->price > o2->price)
    return 1;
  if (o1->quantity < o2->quantity)
    return -1;
  if (o1->quantity > o2->quantity)
    return 1;
  return 0;
}

static int cmp_order_desc(const void *a, const void *b) {
  const Order *o1 = (const Order *)a;
  const Order *o2 = (const Order *)b;
  if (o1->price > o2->price)
    return -1;
  if (o1->price < o2->price)
    return 1;
  if (o1->quantity > o2->quantity)
    return -1;
  if (o1->quantity < o2->quantity)
    return 1;
  return 0;
}

void sort_orders_ascending(OrderArray *orders) {
  qsort(orders->data, orders->size, sizeof(Order), cmp_order_asc);
}

void sort_orders_descending(OrderArray *orders) {
  qsort(orders->data, orders->size, sizeof(Order), cmp_order_desc);
}

int main() {
  // Initialize an order array
  OrderArray buys, sells;
  init_order_array(&buys);
  init_order_array(&sells);

  EventIterator orders_iterator;
  event_iterator_init(&orders_iterator, stdin);

  int order_id_counter = 0;
  Event event;
  while (event_iterator_next(&orders_iterator, &event)) {
    switch (event.type) {
    case EVENT_CREATE: {
      Order order = make_order(
          order_id_counter++,
          event.data.create.side == SIDE_BUY ? ORDER_BUY : ORDER_SELL,
          event.data.create.price, event.data.create.quantity);
      switch (order.order_type) {
      case ORDER_BUY:
        append_order(&buys, order);
        break;
      case ORDER_SELL:
        append_order(&sells, order);
        break;
      }
      break;
    } // end of EVENT_CREATE

    case EVENT_UPDATE: {
      Order *order = order_by_id(&buys, event.data.update.order_id);
      if (!order)
        order = order_by_id(&sells, event.data.update.order_id);
      if (order) {
        order->price = event.data.update.price;
      }
      break;
    }

    case EVENT_REMOVE: {
      // FIXME: Shouldn't search sells if found in buys...
      remove_by_id(&buys, event.data.remove.order_id);
      remove_by_id(&sells, event.data.remove.order_id);
      break;
    }

    case EVENT_BIDS:
      if (buys.size > 0) {
        sort_orders_descending(&buys);
        printf("Bids\n");
        for (size_t i = 0; i < buys.size; i++) {
          printf("\t");
          print_order(order_at_index(&buys, i));
        }
        printf("\n");
      }
      break;

    case EVENT_ASKS:
      if (sells.size > 0) {
        sort_orders_ascending(&sells);
        printf("Asks\n");
        for (size_t i = 0; i < sells.size; i++) {
          printf("\t");
          print_order(order_at_index(&sells, i));
        }
        printf("\n");
      }
      break;
    }
  }

  free_order_array(&buys);
  free_order_array(&sells);

  return 0;
}
