#include <stdio.h>

#include "events.h"
#include "order.h"
#include "order_array.h"
#include "order_map.h"

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

void sort_orders_ascending(OrderArray *orders, OrderMap *order_map) {
  qsort(orders->data, orders->size, sizeof(Order), cmp_order_asc);
  // Rebuild hash table after sorting
  for (size_t i = 0; i < orders->size; i++) {
    order_map_set(order_map, orders->data[i].order_id, orders,
                  &orders->data[i]);
  }
}

void sort_orders_descending(OrderArray *orders, OrderMap *order_map) {
  qsort(orders->data, orders->size, sizeof(Order), cmp_order_desc);
  // Rebuild hash table after sorting
  for (size_t i = 0; i < orders->size; i++) {
    order_map_set(order_map, orders->data[i].order_id, orders,
                  &orders->data[i]);
  }
}

// static void validate_order_map(const OrderMap *map) {
//   for (size_t i = 0; i < map->capacity; i++) {
//     if (map->entries[i].key != EMPTY_KEY) {
//       Order *order = entry_as_order(&map->entries[i]);
//       if (order->order_id != map->entries[i].key) {
//         fprintf(stderr, "Order ID mismatch in order map at index %zu\n", i);
//         fprintf(stderr, "Expected: %d, Found: %d\n", map->entries[i].key,
//                 order->order_id);
//         exit(1);
//       }
//     }
//   }
// }

int main() {
  // Initialize an order array
  OrderArray buys, sells;
  init_order_array(&buys);
  init_order_array(&sells);

  OrderMap order_map;
  order_map_init(&order_map, 16);

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
        order_map_set(&order_map, order.order_id, &buys,
                      &buys.data[buys.size - 1]);
        break;
      case ORDER_SELL:
        append_order(&sells, order);
        order_map_set(&order_map, order.order_id, &sells,
                      &sells.data[sells.size - 1]);
        break;
      }
      break;
    } // end of EVENT_CREATE

    case EVENT_UPDATE: {
      Order *order =
          entry_as_order(order_map_get(&order_map, event.data.update.order_id));
      if (order) {
        order->price = event.data.update.price;
      }
      break;
    }

    case EVENT_REMOVE: {
      Order *order =
          entry_as_order(order_map_get(&order_map, event.data.remove.order_id));
      if (!order)
        continue;

      // The order and the last element will change index and we need to
      // update the map accordingly. So, we map the last item's id to the
      // current order's address (where the last element will be moved).
      if (points_into(&buys, order)) {
        Order *last_order = &buys.data[buys.size - 1];
        order_map_set(&order_map, last_order->order_id, &buys, order);
        remove_by_index(&buys, pointer_index(&buys, order));

      } else if (points_into(&sells, order)) {
        Order *last_order = &sells.data[sells.size - 1];
        order_map_set(&order_map, last_order->order_id, &sells, order);
        remove_by_index(&sells, pointer_index(&sells, order));
      }

      // Delete the order from the map as well.
      order_map_remove(&order_map, event.data.remove.order_id);
      break;
    }

    case EVENT_BIDS:
      if (buys.size > 0) {
        sort_orders_descending(&buys, &order_map);
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
        sort_orders_ascending(&sells, &order_map);
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
  order_map_free(&order_map);
  event_iterator_close(&orders_iterator);

  return 0;
}
