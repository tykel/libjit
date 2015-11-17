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

HEADERS=$(wildcard $(INCLUDE)/*.h) $(wildcard $(SRC)/*.h)
SOURCES=$(wildcard $(SRC)/*.c)
OBJECTS=$(patsubst %.c,%.o,$(SOURCES))

.PHONY: all
all: testjit $(ARCHIVE)

$(ARCHIVE): $(LIB) $(INCLUDE)/libjit.h LICENSE
	$(TAR) -czf $@ $^

testjit: test.c $(LIB) $(HEADERS)
	$(CC) $(CFLAGS) -o $@ test.c $(LIB) -Wl,-rpath=$(DIR) -L./ -ljit

$(LIB): $(OBJECTS) $(HEADERS)
	$(LD) $(LDFLAGS) -o $@ $(OBJECTS)

$(SRC)/%.o: $(SRC)/%.c $(HEADERS)
	$(CC) $(CFLAGS) -fPIC -c -o $@ $<

