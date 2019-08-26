#include <unistd.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
 
int main(int argc, char **argv) {
    int dev;
    char buff[1024];
 
    printf("Device driver test.\n");
 
    dev = open("/dev/virtual_device", O_RDWR);
    printf("dev = %d\n", dev);
 
    write(dev, "1234", 4);
    read(dev, buff, 4);
    printf("read from device: %s\n", buff);
    close(dev);
 
    exit(EXIT_SUCCESS);
}
