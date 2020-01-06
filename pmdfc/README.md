# PMDFC
Persistent Memory Distributed File Cache

## Commands

```modinfo ****.ko``` to see module information

```insmod ****.ko``` or ```make load``` to insert module.

```rmmod ****.ko``` or ```make unload``` to remove module.


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

printk(KERN_INFO "pmdfc: GET PAGE pool_id=%d key=%llu,%llu,%llu index=%ld page=%p\n", pool_id, 
		(long long)oid.oid[0], (long long)oid.oid[1], (long long)oid.oid[2], index, page);

```
[  356.670474] pmdfc: GET PAGE pool_id=1 key=2883636,0,0 index=0 page=0000000094cf7832
[  356.670493] pmdfc: GET PAGE pool_id=1 key=2883636,0,0 index=1 page=00000000c558ba63
[  356.670496] pmdfc: GET PAGE pool_id=1 key=2883636,0,0 index=2 page=000000006957ed95
[  356.670499] pmdfc: GET PAGE pool_id=1 key=2883636,0,0 index=3 page=0000000054507405
```

After run spark benchmark

```
succ_gets :
0
failed_gets :
2161414
puts :
2216211
invalidates :
1278095
```

The first page succeed to put page. But, when succeed get system die.
```
[   49.415492] >> pmdfc: cleancache_register_ops success
[   49.415493] >> pmdfc: cleancache_enabled
[   82.280056] pmdfc: PUT PAGE pool_id=1 key=917931 index=375 page=000000008a88ba8d
[   82.280058] pmdfc: PUT PAGE success
```

## Tries

```
page = page_pool									/* sometimes success */
*page = *page_pool									/* always hang */
mempcy(page, page_pool, sizeof(struct page))		/* always hang */
```


## Errors

* kmod_module_remove_module() could not remove 'ptmem': Device or resource busy.
  - no __exit code (?)

* cannot execute binary file: Exec format erro
  - ??

## Questions

* page pointers are different but key is same why?

* How to print key value?
