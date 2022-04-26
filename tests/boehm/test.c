#include <stdio.h>
// #include "GC_funcs.h"
#include "gc.h"
#define malloc(n) GC_malloc(n)
#define free(n) GC_free(n)

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
void *glob_malloc;
int main(int argc, char **argv)
{
  GC_INIT();

  glob_malloc = malloc(sizeof(double));
  // // lose_malloc();
  struct maze *l3 = nested_mallocs();
  // another_one = l1;
  void *p = malloc(5 * sizeof(int));
  void *chmalloc = malloc(10 * sizeof(char));
  
  free(p);
  
  lose_malloc();

  for (int i = 0; i < 10000; ++i){
    void *p = malloc(sizeof(double));
    void *q = malloc(sizeof(int));
    void *m = malloc(sizeof(char));

    if (i%100 == 0) {
        GC_gcollect();
     }
  }

  return 0;
}
