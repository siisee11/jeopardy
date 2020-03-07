# JEOPARDY
Let's make my system jeopardy!

## how to make device file.

use ndctl to make /dev/dax0.0 with option --mode=devdax

```ndctl create-namespace --force --reconfig=namespace0.0 --mode=devdax --align=2M```

use ndctl to make /dev/pmem0 with option --mode=fsdax

```ndctl create-namespace --force --reconfig=namespace0.0 --mode=fsdax --map=mem```


## Time Stamp
2019/08/18 - create repository.

2019/08/19 - control dax device using POSIX I/O.

2019/09/07 - merge several repository into this.

2019/11/17 - pmdfc (persistent memory disaggregated file cache) directory is added.

2019/12/30 - merge JIFS repository into this.

2020/03/04 - create settings directory (my personal environment settings)


## reference

[How to change kernel version on ubuntu](https://namj.be/2019-12-29-ubuntu-kernel-version/)
[How to use KGDB for debug](https://namj.be/kgdb/2020-01-24-kgdb/)
[How to debug your own kernel module with KGDB](https://namj.be/kgdb/2020-02-21-kgdb-module/)
[About ftrace](https://namj.be/2020-02-10-ftrace/2020-02-10-ftrace/)
