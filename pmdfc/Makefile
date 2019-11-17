CONFIG_MODULE_SIG=n

obj-m := ptmem.o

#ptmem-y := ptmem.o
CFLAGS_super.o := -DDEBUG

all:
	make -C /lib/modules/$(shell uname -r)/build M=$(shell pwd) modules

clean:
	make -C /lib/modules/$(shell uname -r)/build M=$(shell pwd) clean

