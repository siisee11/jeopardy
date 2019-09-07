CFLAGS = $(shell pkg-config --cflags libpmemobj) -O2
LDFLAGS = $(shell pkg-config --libs libpmemobj) -O2
CC = gcc
RM = rm

all: hello_writer hello_reader

%.o: %.c
	$(CC) -c $< -o $@ $(CFLAGS)


hello_writer: writer.o 
	 $(CC) -o $@ $< $(LDFLAGS)

hello_reader: reader.o
	 $(CC) -o $@ $< $(LDFLAGS)

clean: 
	rm -f *.o
	rm hello_reader hello_writer
