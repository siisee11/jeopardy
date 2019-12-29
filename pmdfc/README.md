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

* pmdfc.c is main source code. ptmem.c is deprecated.


## Result

* printk(KERN_INFO "pmdfc: GET PAGE %d %ld %p ino: %ld\n", pool_id, index, page, page->mapping->host->i_ino);

```
[  579.287654] pmdfc: GET PAGE 1 0 0000000010c125fe ino: 2883636
[  579.287670] pmdfc: GET PAGE 1 1 000000001762de72 ino: 2883636
[  579.287672] pmdfc: GET PAGE 1 2 000000002596a2a4 ino: 2883636
[  579.287674] pmdfc: GET PAGE 1 3 000000005dee7a35 ino: 2883636
[  579.290152] pmdfc: GET PAGE 1 4 0000000090527f69 ino: 2883636
[  579.290157] pmdfc: GET PAGE 1 5 00000000e479c066 ino: 2883636
[  579.290159] pmdfc: GET PAGE 1 6 00000000923e518e ino: 2883636
[  579.290161] pmdfc: GET PAGE 1 7 0000000034a8aca2 ino: 2883636
[  579.290164] pmdfc: GET PAGE 1 8 0000000000a63c3f ino: 2883636
[  579.290166] pmdfc: GET PAGE 1 9 000000009c25aecf ino: 2883636
```


## Errors

* kmod_module_remove_module() could not remove 'ptmem': Device or resource busy.
  - no __exit code (?)

* cannot execute binary file: Exec format erro
  - ??
