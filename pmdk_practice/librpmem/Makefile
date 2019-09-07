CFLAGS = $(shell pkg-config --cflags librpmem) -O2
LDFLAGS = $(shell pkg-config --libs librpmem) -O2
CC = gcc
RM = rm

all: rpmem 

%.o: %.c
	$(CC) -c $< -o $@ $(CFLAGS) 

rpmem: rpmem.o 
	 $(CC) -o $@ $< $(LDFLAGS)

clean: 
	rm -f *.o
	rm rpmem
