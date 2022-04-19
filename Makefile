default: libgc.a test
tests: cleanallocs build-subdirs-and-compile-and-run-tests
cleanall: cleanallocs clean

LD_FLAGS += -lunwind-x86_64 -lunwind
LIBALLOCS_PATH = usr/local/src/liballocs
INCLUDE_DIRS += -I/${LIBALLOCS_PATH}/include/
INCLUDE_DIRS += -I/${LIBALLOCS_PATH}/src/
INCLUDE_DIRS += -I/${LIBALLOCS_PATH}/contrib/liballocstool/include/
INCLUDE_DIRS += -I/${LIBALLOCS_PATH}/contrib/libsystrap/contrib/librunt/include/
INCLUDE_DIRS += -I/${LIBALLOCS_PATH}/contrib/libsystrap/include/

# Add Makefile's dir to include dirs
# Assume makefile is at the root dir of project
mkfile_path := $(abspath $(lastword $(MAKEFILE_LIST)))
mkfile_dir := $(subst /Makefile,,$(mkfile_path))
INCLUDE_DIRS += -I$(mkfile_dir)

####
### Building liballocs-gc #####
#####
## Build 3 versions of dlmalloc and archive them with gc functions in libgc.a
# sbrkmalloc.o : only uses sbrk for allocation and custom morecore function used for collection threshold
# pures.o : unedited dlmalloc from https://github.com/stephenrkell/libhighmalloc/blob/master/dlmalloc.c
# mmapmalloc.o : morecore is disabled

DLFLAGS += -g -c
DLFLAGS += -Wall -Wno-unused-label -Wno-comment
DLFLAGS += -O3
DLFLAGS += -fPIC
DLFLAGS += -ffreestanding
sbrkmalloc.o: dlmalloc.c
	allocscc $(DLFLAGS) $(INCLUDE_DIRS) -DHAVE_MMAP=0 -DHAVE_MORECORE=1 -DMORECORE=_morecore -o $@ $^
pures.o: dlmalloc_pure.c
	allocscc $(DLFLAGS) -DHAVE_MMAP=0 -DHAVE_MORECORE=1 -o $@ $^
mmapmalloc.o: dlmalloc.c
	gcc $(DLFLAGS) -DHAVE_MORECORE=0 -o $@ $^

## Build 2 versions of GC_funcs
# DEBUG_TEST can be set if debugging printfs need to be enbaled
# GC_funcs.o : uses sbrk threshold (same macro used as dlmalloc - HAVE_MORECORE) when deciding to garbage collect
# GC_funcs2.o : uses an input counter when deciding to garbage collect. Each malloc() call decrements counter by 1
GC_funcs.o: GC_funcs.c
	gcc ${INCLUDE_DIRS} -DCOUNTER=100 -DDEBUG_TEST=1 -g -c $^
GC_funcs2.o : GC_funcs.c
	gcc ${INCLUDE_DIRS} -DCOUNTER=100 -DDEBUG_TEST=1 -g -o $@ -c $^

## Build 2 versions of libgc
libgc.a: pures.o GC_funcs.o
	$(AR) r "$@" $^
libgc2.a: pures.o GC_funcs2.o
	$(AR) r "$@" $^
LD_FLAGS +=
LD_FLAGS += $(mkfile_dir)/libgc.a
# # LD_FLAGS += -L./boehm/ -lgc # Boehm GC placeholder

include /usr/local/src/liballocs/config.mk
export ELFTIN

## Test program ##
LIBALLOCS_ALLOC_FNS +=
LIBALLOCS_ALLOC_FNS := GC_Malloc(Z)p GC_Calloc(zZ)p GC_Realloc(pZ)p
export LIBALLOCS_ALLOC_FNS
# LIBALLOCS_FREE_FNS := GC_free(p)
# export LIBALLOCS_FREE_FNS
DFLAGS +=
DFLAGS += -Dmalloc=GC_Malloc -Dfree=GC_Free -Dcalloc=GC_Calloc -Drealloc=GC_Realloc

test.o: test.c $(mkfile_dir)/libgc.a
	allocscc ${DFLAGS} ${INCLUDE_DIRS} -c test.c
test : test.o
	allocscc ${LD_FLAGS} ${INCLUDE_DIRS} -o test $< -lallocs
run: test
	LD_PRELOAD=/usr/local/src/liballocs/lib/liballocs_preload.so ./test
	
# allocscc ${INCLUDE_DIRS} -o test test.o /home/user/tasks/libgc.a -L/usr/local/src/liballocs/lib -lallocs -lunwind-x86_64 -lunwind


### --------
### Testing 
#### -------
test_dir := $(mkfile_dir)/tests
test_srcs := $(wildcard $(test_dir)/*.c)
# $(info $(test_srcs))
tests := $(patsubst $(test_dir)/%.c,%,$(test_srcs))
test_file_names := $(patsubst $(test_dir)/%.c,$(test_dir)/%,$(test_srcs))
test_file_execs := $(join $(foreach name,$(test_file_names),$(name)/), $(tests)) # Add slash and join with tests
# $(info $(tests))
# $(info $(test_file_names))
# $(info $(test_file_execs))

# Export these for Makeallocs to use
export test_dir
export LD_FLAGS
export DFLAGS
export INCLUDE_DIRS
# Create subdir -> symbolic link Makeallocs inside subdir -> Run make on this file
build-subdirs-and-compile-and-run-tests:
	for d in $(test_file_names); do \
		mkdir -p $$d; \
		ln -sf ${mkfile_dir}/Makeallocs $$d/Makeallocs; \
		$(MAKE) -f Makeallocs -C $$d run; \
	done
	
cleanallocs:
	find . -name '*.allocstubs.o' -o -name '*.allocstubs.c' -o -name '*.allocstubs.i' -o -name '*.allocstubs.s' -o \
	     -name '*.linked.o' -o -name '*.fixuplog' -o -name '*.o.fixuplog' -o \
		 -name '*.cil.c' -o -name '*.cil.i' -o -name '*.cil.s'  -o \
		 -name '*.i' -o -name '*.i.allocs' | xargs rm -f
	rm -rf $(test_file_names)
	rm -f core

clean:
	rm -f *.so
	rm -f *.a
	rm -f *.o
	rm -f .*.d
	rm -f *.out
