# Makefile for CMAS shell

CC = gcc
CFLAGS = -g -Wall
# main executable file first
TARGET = shell
# object files next
OBJS = main.o tokenify.o plinkedlist.o
# header files next
DEPS = tokenify.h plinkedlist.h
.PHONY : clean

all: $(TARGET)

$(TARGET): $(OBJS)
	$(CC) $(CFLAGS) -o $@ $(OBJS)

.c.o: $(DEPS)
	$(CC) $(CFLAGS) -c $<

clean:
	rm -f $(OBJS) $(TARGET) *~

