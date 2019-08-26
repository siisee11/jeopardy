CONFIG_MODULE_SIG=n

KERNELDIR = /lib/modules/$(shell uname -r)/build
 
obj-m = virtual_device.o
 
KDIR := /lib/modules/$(shell uname -r)/build
PWD := $(shell pwd)
 
default:
	$(MAKE) -C $(KDIR) SUBDIRS=$(PWD) modules
 
clean:
	rm -rf *.ko
	rm -rf *.mod.*
	rm -rf .*.cmd
	rm -rf *.o
