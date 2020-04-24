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

* Networking using tcp/ip. --> it is hard (like concurrent request ?)

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
1
failed_gets :
2161414
puts :
2216211
invalidates :
1278095
```

Succeed

## Tries

```
page = page_pool									/* sometimes success */
*page = *page_pool									/* always hang */
mempcy(page, page_pool, sizeof(struct page))		/* always hang */
```

Next try!

```
pg = kmap_atomic(page)
memcpy(to, page, sizeof(page)
kunmap_atomic(pg)
```

It works!


## Errors

* kmod_module_remove_module() could not remove 'ptmem': Device or resource busy.
  - no __exit code (?)

* cannot execute binary file: Exec format erro
  - ??

## Questions

* page pointers are different but key is same why?
--> Key is from inode number or file handle, so the key is not unique for each page but for file.

* How to print key value?
--> It is easy. See pmdfc.c

* When the linux kenel calls cleancache_get_page?
--> in fs/mpage (mpage_read_page before call bio)

* So, Should we ask pm server whenever get_page called?
--> 



## Reference

[In-kernel networking using tcp ip](https://github.com/abysamross/simple-linux-kernel-tcp-client-server)
[Server side codes](https://github.com/byeongkeonLee/PM_disaggregated_serverside)
