CC=gcc
LD=ld
INCLUDE=./include
SRC=./jit
CFLAGS=-O0 -ggdb -Wall -I$(INCLUDE)
LDFLAGS=-fPIC -shared
OBJECTS=
NAME=jit
DIR=$(shell pwd)
LIB=lib$(NAME).so

SOURCES=$(wildcard $(SRC)/*.c)
OBJECTS=$(patsubst %.c,%.o,$(SOURCES))

.PHONY: all
all: testjit

testjit: main.c $(LIB)
	$(CC) $(CFLAGS) -o $@ $^ -Wl,-rpath=$(DIR) -L./ -ljit

$(LIB): $(OBJECTS)
	$(LD) $(LDFLAGS) -o $@ $^

$(SRC)/%.o: $(SRC)/%.c
	$(CC) $(CFLAGS) -fPIC -c -o $@ $<

