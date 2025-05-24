#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "events.h"

// Helper function
static OrderSide parse_side(const char *side_str) {
  if (strcmp(side_str, "Buy") == 0)
    return SIDE_BUY;
  if (strcmp(side_str, "Sell") == 0)
    return SIDE_SELL;
  fprintf(stderr, "Invalid side: %s\n", side_str);
  exit(EXIT_FAILURE);
}

static Event parse_event(const char *input) {
  Event event;
  char type[16];
  char arg1[16], arg2[16], arg3[16];
  int count = sscanf(input, "%15s %15s %15s %15s", type, arg1, arg2, arg3);

  if (strcmp(type, "CREATE") == 0) {
    if (count != 4) {
      fprintf(stderr, "Invalid CREATE event: %s\n", input);
      exit(EXIT_FAILURE);
    }
    event.type = EVENT_CREATE;
    event.data.create.side = parse_side(arg1);
    event.data.create.quantity = atoi(arg2);
    event.data.create.price = atoi(arg3);
  } else if (strcmp(type, "UPDATE") == 0) {
    if (count != 3) {
      fprintf(stderr, "Invalid UPDATE event: %s\n", input);
      exit(EXIT_FAILURE);
    }
    event.type = EVENT_UPDATE;
    event.data.update.order_id = atoi(arg1);
    event.data.update.price = atoi(arg2);
  } else if (strcmp(type, "REMOVE") == 0) {
    if (count != 2) {
      fprintf(stderr, "Invalid REMOVE event: %s\n", input);
      exit(EXIT_FAILURE);
    }
    event.type = EVENT_REMOVE;
    event.data.remove.order_id = atoi(arg1);
  } else if (strcmp(type, "BIDS") == 0) {
    event.type = EVENT_BIDS;
  } else if (strcmp(type, "ASKS") == 0) {
    event.type = EVENT_ASKS;
  } else {
    fprintf(stderr, "Unknown event type: %s\n", type);
    exit(EXIT_FAILURE);
  }

  return event;
}

bool event_iterator_init(EventIterator *it, FILE *file) {
  it->file = file;
  if (!it->file)
    return false;
  return true;
}
bool event_iterator_next(EventIterator *it, Event *event_out) {
  if (fgets(it->line, LINE_BUF_SIZE, it->file) == NULL) {
    return false; // EOF or error
  }

  // Remove newline if present
  it->line[strcspn(it->line, "\n")] = '\0';

  *event_out = parse_event(it->line);
  return true;
}

void event_iterator_close(EventIterator *it) {
  if (it->file)
    fclose(it->file);
}
