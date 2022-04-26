# liballocs-gc
This is a semi-precise stop-the-world Mark\&Sweep garbage collection service written in C for single-threaded 
workflows, built with liballocs and uses Doug Lea's dlmalloc as a back-end allocator. 
The C interface to the collector consists of four routines that should be called by the user or can be 
supplied as command-line flags at compile-time.

```
#define malloc(n) GC_Malloc(n)
#define free(n) GC_Free(n)
#define realloc(m,n) GC_Realloc(m,n)
#define calloc(m,n) GC_Calloc(m,n)
```

`GC_funcs.c`: crux of the garbage collector. Contains the mark&sweep functions which are used when GC_Malloc deems it appropriate. 

`dlmalloc.c`: the back-end allocator which contains the sbrk threshold implementation that adds a wrapper around sys_alloc and fails when threshold is reached.


## Building liballocs-gc
liballocs needs to be [installed](https://github.com/muffpy/liballocs) first (obviously!). Make sure to use the fork from muffpy's account and not the original. Currently available for x86-64 GNU/Linux systems and tested builds for Debian Buster, Debian Stretch and Ubuntu 18.04, liballocs offers useful run-time metadata such as the program allocation structure and type information at a cheap cost. C files must first be compiled with allocscc 

```
$ allocscc ${INCLUDE_DIRS} -o foo foo.c ${LD_FLAGS}
```

and be run with liballocs preloaded

```
LD_PRELOAD=/usr/local/src/liballocs/lib/liballocs_preload.so ./foo
```

liballocs-gc has only been tested in Debian Buster and Debian Stretch. The collector can be built with a classic Makefile. The procedure could look something like:
```
$ git clone https://github.com/muffpy/liballocs-gc.git
$ cd liballocs-gc
$ make

```
A caveat you may have noticed in the Makefile is the setting the environment with LIBALLOCS_ALLOC_FNS which communicates to `allocscc` to generate allocation wrapper functions for the liballocs-gc interface and instrument calls to this API:

```
include /usr/local/src/liballocs/config.mk
export ELFTIN

LIBALLOCS_ALLOC_FNS := GC_Malloc(Z)p GC_Calloc(zZ)p GC_Realloc(pZ)p
export LIBALLOCS_ALLOC_FNS
```

Users interested in liballocs-gc can then add the generated static library (libgc.a) as an LD_FLAG while compiling with allocscc as follows:

```
$ allocscc ${INCLUDE_DIRS} -o foo foo.c .../liballocs-gc/libgc.a ${OTHER_LD_FLAGS}
```

Running the executable is the same as shown above.

## Allocator and Collector build types
We build 3 versions of dlmalloc and archive them with gc functions.
`sbrkmalloc.o` : only uses sbrk for allocation and custom morecore function used for collection threshold

`pures.o` : unedited dlmalloc from https://github.com/stephenrkell/libhighmalloc/blob/master/dlmalloc.c

`mmapmalloc.o` : morecore is disabled

We build 2 versions of the garbage collector.

`GC_funcs.o` : uses sbrk threshold (same macro used as dlmalloc - HAVE_MORECORE) when deciding to garbage collect

`GC_funcs2.o` : uses an input counter when deciding to garbage collect. Each malloc() call decrements counter by 1

The GC static library may include any combination of these depending on preferences.

## Simple example
You would need to include "GC_funcs.h" in programs that require garbage collection. All `malloc` calls should be replaced with `GC_Malloc` easily defined using a macro. Note that `GC_Free` is a nop.

The following program is a simple example of using liballocs-gc available in test.c.

```
#include <stdio.h>
#include "GC_funcs.h"

#define malloc(n) GC_Malloc(n)

/* -------------------------------- Test program ----------------------------- */

int main(int argc, char **argv)
{
  int* reachable = (int*) malloc(sizeof(void*));

  for (int i = 0; i < 200000; ++i){
    void *p = malloc(sizeof(int));
    void *q = malloc(sizeof(double));
    void *m = malloc(sizeof(char));
  }
  exp_collect(); /* Call collector */
  
  /* Print out remaining object addresses and uniqtypes. Should show the 'reachable' object.
      May also show some garbage.
  */
  inspect_allocs();

  printf("SUCCESS \n");

  return 0;
}

```

## Useful macros and functions in `GC_funcs.c`
Note that these macros can also be passed as a command-line flag (-D_macro_) while comiling the program file.


`#NOGC`: can be used to disable all collection thresholds and gc in general.

`#DEBUG_TEST`: setting this to 1 enables all the debug_printf() statements in GC_funcs.c which may prove useful for debugging the gc step-by-step.

`#COUNTER`: a collection threshold which triggers gc when counter reaches 0. Each GC_Malloc() decrements this counter by 1.

`#HAVE_MORECORE`: a collection threshold which triggers gc when 10KB of new heap space was obtained from sbrk().

`#STATS`: setting this to 1 counts the number of (successful) malloc() (or allocations) and free() (or deallocations of garbage).

`exp_collect()`: can be used to explicitly force gc.

`timed_collect()`: same as exp_collect() but times the gc.

## Tests
Add new C files to the /tests directory and add brief decription below. Test files can be compiled either from the root directory:

```
$ make tests
```

or using the Makefile inside the tests directory (which the previous command basically) that offers more control over compilation and execution paramters:

```
$ cd tests
$ make
```

`nest.c`: allocates structs with intermingled pointers and 'loses' 600000 malloc'd objects

`glibc-simple.c`: glibc benchmark used to test performance of malloc() and free() by making large number of allocations of varying sizes.

`binarytrees.c`: adapted BohemGC Bench test which allocates large numbers of fully binary trees and walks them top-down.

`cycles.c`: allocates cyclic linked list of garbage.

`fasta.c`: borrowed from Benchmark Game. Used to generate DNA sequences in the FASTA format from a small starting-point sequence or a weighted random selection. Mostly sued to generate input for knucleotide.c

`knucleotide.c`: borrowed from Benchmark Game. Used to parse and decode DNA sequences. A unique allocation pattern is the repeated use of realloc's between malloc calls which stress tests the underlying bitmap implementation responsible for "reindexing" the bit entries and ensuring the correct relocation of the inserts.
