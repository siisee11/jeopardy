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

2019/11/17 - pmdfc (persistent memory distributed file cache) directory is added.
