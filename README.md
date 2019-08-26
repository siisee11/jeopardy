# JEOPARDY
Let's make my system jeopardy!

## how to make device file.

use ndctl to make /dev/dax0.0 with option --mode=devdax

```ndctl create-namespace --force --reconfig=namespace0.0 --mode=devdax --align=2M```

use ndctl to make /dev/pmem0 with option --mode=fsdax

```ndctl create-namespace --force --reconfig=namespace0.0 --mode=fsdax --map=mem```

