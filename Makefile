CC=gcc
LD=ld
TAR=tar

INCLUDE=./include
SRC=./jit
CFLAGS=-O0 -ggdb -Wall -Wno-unused-label -I$(INCLUDE)
LDFLAGS=-fPIC -shared
OBJECTS=
NAME=jit
DIR=$(shell pwd)
LIB=lib$(NAME).so
ARCHIVE=lib$(NAME)-linux-x86_64.tar.gz

SOURCES=$(wildcard $(SRC)/*.c)
OBJECTS=$(patsubst %.c,%.o,$(SOURCES))

.PHONY: all
all: testjit $(ARCHIVE)

$(ARCHIVE): $(LIB) $(INCLUDE)/jit.h LICENSE
	$(TAR) -czf $@ $^

testjit: main.c $(LIB)
	$(CC) $(CFLAGS) -o $@ $^ -Wl,-rpath=$(DIR) -L./ -ljit

$(LIB): $(OBJECTS)
	$(LD) $(LDFLAGS) -o $@ $^

$(SRC)/%.o: $(SRC)/%.c
	$(CC) $(CFLAGS) -fPIC -c -o $@ $<

