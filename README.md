# liballocs-gc

To run the code in the liballocs Debian container:

```
allocscc -I/usr/local/src/liballocs/include -I/usr/local/src/liballocs/contrib/liballocstool/include -I/usr/local/src/liballocs/contrib/libsystrap/contrib/librunt/include -o test test.c

LD_PRELOAD=/usr/local/src/liballocs/lib/liballocs_preload.so ./test
```
