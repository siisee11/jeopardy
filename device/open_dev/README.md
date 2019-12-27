# Open persistent memory as file
Not done yet.

fsdax works, but devdax doesn't.


## FSDAX RESULT

```
root@apache2:/home/siisee11/git/jeopardy/device/open_dev# ./open_dev
On start, mptr points to 0x7F1A146FE000.
mptr points to 0x7F1A146FE000. *mptr = 0x4A
mptr points to 0x7F1A146FE000. *mptr = 0x61
in dev, a c
root@apache2:/home/siisee11/git/jeopardy/device/open_dev# ./open_dev
On start, mptr points to 0x7FEC2CF21000.
mptr points to 0x7FEC2CF21000. *mptr = 0x61
mptr points to 0x7FEC2CF21000. *mptr = 0x61
in dev, a c
root@apache2:/home/siisee11/git/jeopardy/device/open_dev# ./open_dev
On start, mptr points to 0x7F9DF5707000.
mptr points to 0x7F9DF5707000. *mptr = 0x61
mptr points to 0x7F9DF5707000. *mptr = 0x61
in dev, a c
root@apache2:/home/siisee11/git/jeopardy/device/open_dev# ./open_dev
On start, mptr points to 0x7F4389FA6000.
mptr points to 0x7F4389FA6000. *mptr = 0x61
mptr points to 0x7F4389FA6000. *mptr = 0x61
in dev, a c
root@apache2:/home/siisee11/git/jeopardy/device/open_dev# ./open_dev
On start, mptr points to 0x7F8711EFB000.
mptr points to 0x7F8711EFB000. *mptr = 0x61
mptr points to 0x7F8711EFB000. *mptr = 0x61
in dev, a c
```

## how to make device file.

use ndctl to make /dev/dax0.0 with option --mode=devdax

```ndctl create-namespace --force --reconfig=namespace0.0 --mode=devdax --align=2M```

use ndctl to make /dev/pmem0 with option --mode=fsdax

```ndctl create-namespace --force --reconfig=namespace0.0 --mode=fsdax --map=mem```

