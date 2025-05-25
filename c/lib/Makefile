CC = cc
AR = ar
#CFLAGS = -Wall -Wextra -O2 -I.
CFLAGS = -Wall -Wextra -g -O0 -fno-omit-frame-pointer -I.
SRC = $(wildcard *.c)
OBJ = $(SRC:.c=.o)
LIB = liborderbook.a

.PHONY: all clean

all: $(LIB)

$(LIB): $(OBJ)
	$(AR) rcs $@ $^

%.o: %.c
	$(CC) $(CFLAGS) -c $< -o $@

clean:
	rm -f $(OBJ) $(LIB)
