CC = cc
AR = ar
CFLAGS = -Wall -Wextra -O2 -I. -I../lib 
SRC = $(wildcard *.c)
OBJ = $(SRC:.c=.o)
BIN = main

.PHONY: all clean

all: $(BIN)

$(BIN): $(OBJ)
	$(MAKE) -C ../lib liborderbook.a
	$(CC) $(CFLAGS) -L../lib -lorderbook $^ -o $@

%.o: %.c
	$(CC) $(CFLAGS) -c $< -o $@

clean:
	rm -f $(OBJ) $(LIB)
