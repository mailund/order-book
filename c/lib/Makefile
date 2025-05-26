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
LIB = liborderbook.a

.PHONY: all clean

all: $(LIB)

$(LIB): $(OBJ)
	$(AR) rcs $@ $^

%.o: %.c
	$(CC) $(CFLAGS) -c $< -o $@

clean:
	rm -f $(OBJ) $(LIB)
