CC = cc
AR = ar
CFLAGS = -Wall -Wextra -O2 -I. -I.. 
SRC = $(wildcard *.c)
OBJ = $(SRC:.c=.o)
BIN = unsorted_lists

.PHONY: all clean

all: $(BIN)

$(BIN): $(OBJ)
	$(MAKE) -C .. liborderbook.a
	$(CC) $(CFLAGS) -L.. -lorderbook $^ -o $@

%.o: %.c
	$(CC) $(CFLAGS) -c $< -o $@

clean:
	rm -f $(OBJ) $(LIB)
