BUILD ?= release

ifeq ($(BUILD), profile)
CFLAGS = -fsanitize=address -Wall -Wextra -g -O0 -fno-omit-frame-pointer -I. -I../lib -DPROFILING
else
CFLAGS = -Wall -Wextra -O2 -I. -I../lib
endif

CC = cc
AR = ar

SRC = $(wildcard *.c)
OBJ = $(SRC:.c=.o)
BIN = main

.PHONY: all clean

all: main bytes

main: main.o
	$(MAKE) -C ../lib liborderbook.a
	$(CC) $(CFLAGS) -L../lib -lorderbook $^ -o $@

bytes: main_bytes.o
	$(MAKE) -C ../lib liborderbook.a
	$(CC) $(CFLAGS) -L../lib -lorderbook $^ -o $@

%.o: %.c
	$(CC) $(CFLAGS) -c $< -o $@

clean:
	rm -f $(OBJ) $(LIB)
