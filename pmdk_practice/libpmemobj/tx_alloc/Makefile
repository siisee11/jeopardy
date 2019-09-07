CFLAGS = $(shell pkg-config --cflags libpmemobj) -O2
LDFLAGS = $(shell pkg-config --libs libpmemobj) -O2
CC = gcc
RM = rm

all: rect_calc

%.o: %.c
	$(CC) -c $< -o $@ $(CFLAGS)


rect_calc: rect_calc.o 
	 $(CC) -o $@ $< $(LDFLAGS)

clean: 
	rm -f *.o
	rm hello_reader hello_writer
