CC=gcc
CFLAGS=-g -Wall -pthread
TARGETS=procshare threadshare mtlist

all: $(TARGETS)

procshare: procshare.o
	$(CC) $(CFLAGS) -o $@ procshare.o

threadshare: threadshare.o
	$(CC) $(CFLAGS) -o $@ threadshare.o

mtlist: list.o mtlist.o
	$(CC) $(CFLAGS) -o $@ list.o mtlist.o

.PHONY: clean
clean:
	rm -f *.o $(TARGETS) list.o

.c.o:
	$(CC) -c $(CFLAGS) $<
