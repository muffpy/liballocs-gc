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