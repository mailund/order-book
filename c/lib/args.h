#pragma once

#include <stdbool.h>

typedef struct {
  bool silent;
  const char *input_file;
} Config;

void parse_args(Config *cfg, int argc, char *argv[]);
