CFLAGS = $(shell pkg-config --cflags libpmem) -O2
LDFLAGS = $(shell pkg-config --libs libpmem) -O2
CC = gcc
RM = rm

all: open_dev

%.o: %.c
	$(CC) -c $< -o $@ $(CFLAGS) 

open_dev: open_dev.o
	 $(CC) -o $@ $< $(LDFLAGS)

clean: 
	rm -f *.o
	rm open_dev
