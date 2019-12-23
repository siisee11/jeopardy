# PMDFC
Persistent Memory Distributed File Cache

## Commands

```modinfo ****.ko``` to see module information

```insmod ****.ko``` to insert module.

```rmmod ****.ko``` to remove module.


## What I did

* How to add 'tmem' to kernel parameters?
  - Add 'tmem=1' to /etc/sysctl.conf file. 
    -```sysctl -a | grep tmem``` to check kernel parameter
  - Add parameter to GRUB_CMDLINE_LINUX_DEFAULT of ```/etc/default/grub```
    -```cat /proc/cmdline``` to check kernel command line parameters.
    - IDK

* insmod works. ```dmesg``` to see log

## Errors

* kmod_module_remove_module() could not remove 'ptmem': Device or resource busy.
  - no __exit code (?)

* cannot execute binary file: Exec format erro
  - ??
