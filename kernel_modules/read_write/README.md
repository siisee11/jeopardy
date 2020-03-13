# Read file from kernel

## Env

linux5.3

## Kread

Read and print string in 'etc/issue' and write 'foo' to '/etc/temp'

```
$ make
$ insmod kread.ko
$ rmmod kread
```
