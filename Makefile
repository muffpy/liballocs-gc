default: libgc.a

LD_FLAGS += -lunwind-x86_64 -lunwind
LIBALLOCS_PATH = usr/local/src/liballocs
INCLUDE_DIRS += -I/${LIBALLOCS_PATH}/include/
INCLUDE_DIRS += -I/${LIBALLOCS_PATH}/src/
INCLUDE_DIRS += -I/${LIBALLOCS_PATH}/contrib/liballocstool/include/
INCLUDE_DIRS += -I/${LIBALLOCS_PATH}/contrib/libsystrap/contrib/librunt/include/
INCLUDE_DIRS += -I/${LIBALLOCS_PATH}/contrib/libsystrap/include/

# Build 3 versions of dlmalloc and archive them with gc functions in libgc.a
# pdlmalloc.o - disable mmap() for sys alloc, create functions with dl prefix and contains morecore wrapper for gc
# nopdlmalloc.o - disable sbrk() and creat functions without dl prefix
# dlmallocpure.o - disable mmap() fallback and create functions with dl prefix
DLFLAGS += -g -c
DLFLAGS += -Wall -Wno-unused-label -Wno-comment
DLFLAGS += -O3
DLFLAGS += -fPIC
DLFLAGS += -ffreestanding
pdlmalloc.o: dlmalloc.c
	gcc $(DLFLAGS) -DHAVE_MMAP=0 -DUSE_DL_PREFIX -o $@ $^
dlmallocpure.o: dlmalloc_pure.c
	gcc $(DLFLAGS) -DHAVE_MMAP=0 -DUSE_DL_PREFIX -o $@ $^
nopdlmalloc.o: dlmalloc.c
	gcc $(DLFLAGS) -DHAVE_MORECORE=0 -o $@ $^
GC_funcs.o: GC_funcs.c
	gcc ${INCLUDE_DIRS} -c $^
libgc.a: nopdlmalloc.o pdlmalloc.o GC_funcs.o
	$(AR) r "$@" $^
LD_FLAGS += -L. -lgc
# # LD_FLAGS += -L./boehm/ -lgc # Boehm GC placeholder

## Test program ##
LIBALLOCS_ALLOC_FNS := GC_malloc(Z)p GC_calloc(zZ)p GC_realloc(pZ)p
export LIBALLOCS_ALLOC_FNS
# LIBALLOCS_FREE_FNS := GC_free(p)
# export LIBALLOCS_FREE_FNS
DFLAGS +=
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