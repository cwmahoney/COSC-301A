CC=gcc
CFLAGS=-g -Wall

all: broken1 broken2

broken1: broken1.o
	$(CC) $(CFLAGS) -o $@ broken1.o

broken2: broken2.o
	$(CC) $(CFLAGS) -o $@ broken2.o

.PHONY: clean
clean:
	rm -f *.o broken?

.c.o:
	$(CC) -c $(CFLAGS) $<
