CFLAGS = $(shell pkg-config --cflags libpmem) -O2
LDFLAGS = $(shell pkg-config --libs libpmem) -O2
CC = gcc
RM = rm

all: test

%.o: %.c
	$(CC) -c $< -o $@ $(CFLAGS) 

test: test.o
	 $(CC) -o $@ $< $(LDFLAGS)

clean: 
	rm -f *.o
	rm test
