default: libgc.a

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
libgc.a: nopdlmalloc.o dlmallocpure.o GC_funcs.o
	$(AR) r "$@" $^
LD_FLAGS +=
LD_FLAGS += -L$(mkfile_dir) -lgc
# # LD_FLAGS += -L./boehm/ -lgc # Boehm GC placeholder

## Test program ##
LIBALLOCS_ALLOC_FNS +=
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


### Testing ####
test_dir := $(mkfile_dir)/tests
test_srcs := $(wildcard $(test_dir)/*.c)
$(info $(test_srcs))
tests := $(patsubst $(test_dir)/%.c,%,$(test_srcs))
test_file_names := $(patsubst $(test_dir)/%.c,$(test_dir)/%,$(test_srcs))
test_file_execs := $(join $(foreach name,$(test_file_names),$(name)/), $(tests)) # Add slash and join with tests
$(info $(tests))
$(info $(test_file_names))
$(info $(test_file_execs))

$(tests):
	mkdir -p $(test_dir)/$@
# CC := allocscc
# CFLAGS += INCLUDE_DIRS
# CFLAGS += DFLAGS

$(test_file_execs): $(test_srcs)| $(tests)
	cd $(@D) && \
	allocscc ${DFLAGS} ${LD_FLAGS} ${INCLUDE_DIRS} $< -o $(@F) && \
	cd $(mkfile_dir)	

runtests: $(test_file_execs)
	for x in $(test_file_execs); do LD_PRELOAD=/usr/local/src/liballocs/lib/liballocs_preload.so $$x; done

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