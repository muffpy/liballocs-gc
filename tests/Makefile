.PHONY: defualt clean cleanall
default: simple cycles binarytrees fasta knucleotide
run: runr runc runb runk runf
cleanall: cleanallocs clean

LD_FLAGS += -lunwind-x86_64 -lunwind
LIBALLOCS_PATH = usr/local/src/liballocs
INCLUDE_DIRS += -I/${LIBALLOCS_PATH}/include/
INCLUDE_DIRS += -I/${LIBALLOCS_PATH}/src/
INCLUDE_DIRS += -I/${LIBALLOCS_PATH}/contrib/liballocstool/include/
INCLUDE_DIRS += -I/${LIBALLOCS_PATH}/contrib/libsystrap/contrib/librunt/include/
INCLUDE_DIRS += -I/${LIBALLOCS_PATH}/contrib/libsystrap/include/

LD_FLAGS +=
LD_FLAGS += ../libgc2.a

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

glibc-simple.o: glibc-simple.c
	echo "Compiling $< with allocscc" && \
	allocscc ${DFLAGS} ${INCLUDE_DIRS} -c $<
simple: glibc-simple.o
	allocscc ${LD_FLAGS} ${INCLUDE_DIRS} -o simple $< -lallocs
runr: simple
	LD_PRELOAD=/usr/local/src/liballocs/lib/liballocs_preload.so ./simple 128
timesimple: simple
	./timeit -s -r 10 "LD_PRELOAD=/usr/local/src/liballocs/lib/liballocs_preload.so ./simple 128"

cycles.o: cycles.c
	echo "Compiling $< with allocscc" && \
	allocscc ${DFLAGS} ${INCLUDE_DIRS} -c $<
cycles: cycles.o
	allocscc ${LD_FLAGS} ${INCLUDE_DIRS} -o cycles $< -lallocs
runc: cycles
	LD_PRELOAD=/usr/local/src/liballocs/lib/liballocs_preload.so ./cycles
timecycles: cycles
	./timeit -s -r 10 "LD_PRELOAD=/usr/local/src/liballocs/lib/liballocs_preload.so ./cycles"

binarytrees.o: binarytrees.c
	echo "Compiling $< with allocscc" && \
	allocscc ${DFLAGS} ${INCLUDE_DIRS} -c $<
binarytrees: binarytrees.o
	allocscc ${LD_FLAGS} ${INCLUDE_DIRS} -o binarytrees $< -lallocs
runb: binarytrees
	LD_PRELOAD=/usr/local/src/liballocs/lib/liballocs_preload.so ./binarytrees 17
timebinarytrees: binarytrees
	./timeit -s -r 10 "LD_PRELOAD=/usr/local/src/liballocs/lib/liballocs_preload.so ./binarytrees 17"

knucleotide.o: knucleotide.c
	echo "Compiling $< with allocscc" && \
	allocscc ${DFLAGS} ${INCLUDE_DIRS} -c $<
knucleotide : knucleotide.o
	allocscc ${LD_FLAGS} ${INCLUDE_DIRS} -o knucleotide $< -lallocs
runk: knucleotide
	LD_PRELOAD=/usr/local/src/liballocs/lib/liballocs_preload.so ./knucleotide 0 < nucleotideinput.txt

fasta.o: fasta.c
	echo "Compiling $< with allocscc" && \
	allocscc ${DFLAGS} ${INCLUDE_DIRS} -c $<
fasta: fasta.o
	allocscc ${LD_FLAGS} ${INCLUDE_DIRS} -o fasta $< -lallocs
runf: fasta
	LD_PRELOAD=/usr/local/src/liballocs/lib/liballocs_preload.so ./fasta 25000 > nucleotideinput.txt


cleanallocs:
	find . -name '*.allocstubs.o' -o -name '*.allocstubs.c' -o -name '*.allocstubs.i' -o -name '*.allocstubs.s' -o \
	     -name '*.linked.o' -o -name '*.fixuplog' -o -name '*.o.fixuplog' -o \
		 -name '*.cil.c' -o -name '*.cil.i' -o -name '*.cil.s'  -o \
		 -name '*.i' -o -name '*.i.allocs' | xargs rm -f
	rm -rf $(test_file_names)
	rm -f core
	rm -f simple
	rm -f knucleotide
	rm -f fasta
	rm -f binarytrees

clean:
	rm -f *.so
	rm -f *.a
	rm -f *.o
	rm -f .*.d
	rm -f *.out
