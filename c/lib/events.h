#pragma once

#include <stdbool.h>
#include <stdio.h>

typedef enum {
  EVENT_CREATE,
  EVENT_UPDATE,
  EVENT_REMOVE,
  EVENT_BIDS,
  EVENT_ASKS
} EventType;

typedef enum { SIDE_BUY, SIDE_SELL } OrderSide;

typedef struct {
  OrderSide side;
  int quantity;
  int price;
} CreateOrder;

typedef struct {
  int order_id;
  int price;
} UpdateOrder;

typedef struct {
  int order_id;
} RemoveOrder;

typedef struct {
  EventType type;
  union {
    CreateOrder create;
    UpdateOrder update;
    RemoveOrder remove;
    // BIDS and ASKS have no data
  } data;
} Event;

// FIXME: This is probably good enough for jazz...
#define LINE_BUF_SIZE 256
typedef struct {
  FILE *file;
  char line[LINE_BUF_SIZE];
} EventIterator;

bool event_iterator_init(EventIterator *it, FILE *file);
bool event_iterator_next(EventIterator *it, Event *event_out);
void event_iterator_close(EventIterator *it);