CC=gcc
LD=ld
INCLUDE=./include
SRC=./jit
CFLAGS=-O2 -Wall -fPIC -I$(INCLUDE)
LDFLAGS=-fPIC -shared
OBJECTS=
NAME=jit
LIB=lib$(NAME).so

SOURCES=$(wildcard $(SRC)/*.c)
OBJECTS=$(patsubst %.c,%.o,$(SOURCES))

.PHONY: all
all: $(LIB)

$(LIB): $(OBJECTS)
	$(LD) $(LDFLAGS) -o $@ $^

$(SRC)/%.o: $(SRC)/%.c
	$(CC) $(CFLAGS) -c -o $@ $<

