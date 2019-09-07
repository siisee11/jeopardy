# Open persistent memory as file
Not done yet.

## how to make device file.

use ndctl to make /dev/dax0.0 with option --mode=devdax

```ndctl create-namespace --force --reconfig=namespace0.0 --mode=devdax --align=2M```

use ndctl to make /dev/pmem0 with option --mode=fsdax

```ndctl create-namespace --force --reconfig=namespace0.0 --mode=fsdax --map=mem```

