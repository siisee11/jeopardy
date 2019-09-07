CFLAGS = $(shell pkg-config --cflags libpmemobj) -O2
LDFLAGS = $(shell pkg-config --libs libpmemobj) -O2
CC = gcc
RM = rm

all: lists

%.o: %.c
	$(CC) -c $< -o $@ $(CFLAGS)


lists: lists.o 
	 $(CC) -o $@ $< $(LDFLAGS)

clean: 
	rm -f *.o
	rm lists
