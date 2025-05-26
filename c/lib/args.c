#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "args.h"

void parse_args(Config *cfg, int argc, char *argv[]) {
  cfg->silent = false;
  cfg->input_file = NULL;

  for (int i = 1; i < argc; i++) {
    if (strcmp(argv[i], "--silent") == 0 || strcmp(argv[i], "-s") == 0) {
      cfg->silent = true;
    } else if ((strcmp(argv[i], "--input") == 0 ||
                strcmp(argv[i], "-i") == 0) &&
               i + 1 < argc) {
      cfg->input_file = argv[++i];
    } else {
      fprintf(stderr, "Unknown argument: %s\n", argv[i]);
      fprintf(stderr, "Usage: %s [--silent|-s] [--input|-i <file>]\n", argv[0]);
      exit(EXIT_FAILURE);
    }
  }
}
