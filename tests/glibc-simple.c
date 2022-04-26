/* Benchmark malloc and free functions.
   Copyright (C) 2019-2022 Free Software Foundation, Inc.
   This file is part of the GNU C Library.
   The GNU C Library is free software; you can redistribute it and/or
   modify it under the terms of the GNU Lesser General Public
   License as published by the Free Software Foundation; either
   version 2.1 of the License, or (at your option) any later version.
   The GNU C Library is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
   Lesser General Public License for more details.
   You should have received a copy of the GNU Lesser General Public
   License along with the GNU C Library; if not, see
   <https://www.gnu.org/licenses/>.  */

#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <malloc.h>
#include <sys/resource.h>
#include <time.h> /* Modified code */
// #include "boehm/gc.h"
// #define malloc(n) GC_malloc(n)
#include "../GC_funcs.h"


/* Benchmark the malloc/free performance of a varying number of blocks of a
   given size.  This enables performance tracking of the t-cache and fastbins.
   It tests 3 different scenarios: single-threaded using main arena,
   multi-threaded using thread-arena, and main arena with SINGLE_THREAD_P
   false.  */

#define NUM_ITERS 200000
#define NUM_ALLOCS 4
#define MAX_ALLOCS 1600

typedef struct
{
  size_t iters;
  size_t size;
  int n;
} malloc_args;

static void
do_benchmark (malloc_args *args)
{
  size_t iters = args->iters;
  size_t size = args->size;
  int n = args->n;

  for (int j = 0; j < iters; j++)
    {
      for (int i = 0; i < n; i++)
	    (void*) malloc (size);

    }
}

static malloc_args tests[3][NUM_ALLOCS];
static int allocs[NUM_ALLOCS] = { 25, 100, 400, MAX_ALLOCS };

void
bench (unsigned long size)
{
  size_t iters = NUM_ITERS;
  clock_t start, stop;

  start = clock();
  for (int t = 0; t < 3; t++){
    for (int i = 0; i < NUM_ALLOCS; i++)
      {
        tests[t][i].n = allocs[i];
        tests[t][i].size = size;
        tests[t][i].iters = iters / allocs[i];

        do_benchmark (&tests[0][i]);
      }
      // exp_collect();
      timed_collect();
      // GC_gcollect(); /*Boehm gc*/
  }
  stop = clock();

  printf("Timing (ms) %f\n", (((double)stop-start)/CLOCKS_PER_SEC)*1000);
  // inspect_allocs();
}

static void usage (const char *name)
{
  fprintf (stderr, "%s: <alloc_size>\n", name);
  exit (1);
}

int
main (int argc, char **argv)
{
  // GC_INIT();

  long val = 16;
  if (argc == 2)
    val = strtol (argv[1], NULL, 0);

  if (argc > 2 || val <= 0)
    usage (argv[0]);

  bench (val);

  return 0;
}