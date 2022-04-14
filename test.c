#include <stdio.h>
#include "GC_funcs.h"
#include "boehm/gc.h"

/* -------------------------------- Test program ----------------------------- */

void lose_malloc(){
  // Create and lose some mallocs
  void *l = malloc(sizeof(double));
  void *p = malloc(2 * sizeof(double));
  void *m = malloc(3 * sizeof(int));
  void *q = malloc(4 * sizeof(int));
  void *n = malloc(5 * sizeof(int));
}
struct maze {
  int num;
  double num2;
  char *name;
  struct maze *inner;
};
struct maze *nested_mallocs() {
  struct maze *l1;
  struct maze *l2;
  l1 = malloc(sizeof(struct maze));
  l2 = malloc(sizeof(struct maze));
  l1->num = malloc(20);
  l1->num2 = 14;
  l1->inner = l2;
  l2->name = "inner";
  return l1;
}
//globs
void *GLOBAL_VAR_malloc;
int glob;
int main(int argc, char **argv)
{
  glob = 9 * 20;
  GLOBAL_VAR_malloc = malloc(sizeof(double));
  // // lose_malloc();
  struct maze *l3 = nested_mallocs();
  // another_one = l1;
  void *p = malloc(5 * sizeof(int));
  void *chmalloc = malloc(10 * sizeof(char));
  
  free(p);

  lose_malloc();

  for (int i = 0; i < 1000; ++i){
    void *p = malloc(sizeof(double));
  }

  return 0;
}
