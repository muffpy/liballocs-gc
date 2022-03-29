default: libgc.a

LD_FLAGS += -lunwind-x86_64 -lunwind
LIBALLOCS_PATH = usr/local/src/liballocs
INCLUDE_DIRS += -I/${LIBALLOCS_PATH}/include/
INCLUDE_DIRS += -I/${LIBALLOCS_PATH}/src/
INCLUDE_DIRS += -I/${LIBALLOCS_PATH}/contrib/liballocstool/include/
INCLUDE_DIRS += -I/${LIBALLOCS_PATH}/contrib/libsystrap/contrib/librunt/include/
INCLUDE_DIRS += -I/${LIBALLOCS_PATH}/contrib/libsystrap/include/

# Build 2 versions of dlmalloc and archive them with gc functions in libgc.a
# - disable mmap() fallback and functions with dl prefix : dlmalloc.o
# - disable sbrk() and without dl prefix : nodlmalloc.o
DLFLAGS += -g -c
DLFLAGS += -Wall -Wno-unused-label -Wno-comment
DLFLAGS += -O3
DLFLAGS += -fPIC
DLFLAGS += -ffreestanding
dlmalloc.o: dlmalloc.c
	$(CC) -o $@ $(DLFLAGS) -DHAVE_MMAP=0 -DUSE_DL_PREFIX=1 $^
nodlmalloc.o: dlmalloc.c
	$(CC) -o $@ $(DLFLAGS) -DHAVE_MORECORE=0 -DUSE_DL_PREFIX=0 $^
GC_funcs.o: GC_funcs.c
	$(CC) ${INCLUDE_DIRS} -c $^
libgc.a: nodlmalloc.o dlmalloc.o GC_funcs.o
	$(AR) r "$@" $^
LD_FLAGS += -L. -lgc
# LD_FLAGS += -L./boehm/ -lgc # Boehm GC placeholder

## Test program ##
LIBALLOCS_ALLOC_FNS := GC_malloc(Z)p GC_calloc(zZ)p GC_realloc(pZ)p
export LIBALLOCS_ALLOC_FNS
# LIBALLOCS_FREE_FNS := GC_free(p)
# export LIBALLOCS_FREE_FNS
DFLAGS += -Dmalloc=GC_malloc -Dfree=GC_free -Dcalloc=GC_calloc -Drealloc=GC_realloc
test : test.c
	allocscc ${DFLAGS} ${LD_FLAGS} ${INCLUDE_DIRS} -o test test.c

run: test
	LD_PRELOAD=/usr/local/src/liballocs/lib/liballocs_preload.so ./test

clean:
	rm -f *.so
	rm -f *.a
	rm -f *.o
	rm -f .*.d