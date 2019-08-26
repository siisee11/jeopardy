# Virtual Device Driver.

## how to run this.

To make character device in /dev, type this.

`sudo mknod /dev/virtual_device c 255 0`

255 is major number and 0 is minor number.

`sudo insmod virtual_device.ko`

Let's go to test directory and run test.

`sudo ./test`

make sure you have to run with sudo.

