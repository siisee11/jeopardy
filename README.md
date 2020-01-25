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


## reference

[Change Kernel on ubuntu](https://medium.com/@siisee111/virtual-ubuntu-%ED%8A%B9%EC%A0%95-%EB%B2%84%EC%A0%84%EC%9C%BC%EB%A1%9C-%EC%BB%A4%EB%84%90-%EB%B2%84%EC%A0%84-%EB%B0%94%EA%BE%B8%EA%B8%B0-e5555ffc2121)
