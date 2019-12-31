CONFIG_MODULE_SIG=n

obj-m := jifs.o

jifs-y := super.o dir.o namei.o inode.o file.o
CFLAGS_super.o := -DDEBUG

all:
	make -C /lib/modules/$(shell uname -r)/build M=$(shell pwd) modules

clean:
	make -C /lib/modules/$(shell uname -r)/build M=$(shell pwd) clean
