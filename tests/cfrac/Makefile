default: cfrac
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

libgc_dir := $(subst tests/cfrac/Makefile,,$(mkfile_path))
LD_FLAGS += $(libgc_dir)libgc2.a

include /usr/local/src/liballocs/config.mk
export ELFTIN

## Test program ##
LIBALLOCS_ALLOC_FNS +=
LIBALLOCS_ALLOC_FNS := GC_Malloc(Z)p GC_Calloc(zZ)p GC_Realloc(pZ)p
export LIBALLOCS_ALLOC_FNS
# LIBALLOCS_FREE_FNS := GC_free(p)
# export LIBALLOCS_FREE_FNS
DFLAGS +=
DFLAGS += -Dmalloc=GC_Malloc

cfrac: cfrac.c pops.c pconst.c pio.c pabs.c pneg.c pcmp.c podd.c phalf.c padd.c psub.c pmul.c pdivmod.c psqrt.c ppowmod.c atop.c ptoa.c itop.c utop.c ptou.c errorp.c pfloat.c pidiv.c pimod.c picmp.c primes.c pcfrac.c pgcd.c
	allocscc ${DFLAGS} ${LD_FLAGS} ${INCLUDE_DIRS} -DIGNOREFREE -g -o cfrac $? -lm -lallocs
run: cfrac
	LD_PRELOAD=/usr/local/src/liballocs/lib/liballocs_preload.so ./cfrac 17545186520507317056371524

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

	rm -f cfrac