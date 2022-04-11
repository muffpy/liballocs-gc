#include <allocs.h>
#include <stdio.h>
#include <inttypes.h>
#include "relf.h" /* Contains fake_dladdr and _r_debug */
#include "heap_index.h"
#include "GC_funcs.h"
// #include "boehm/gc.h"

/* -------------------------------- Test program ----------------------------- */

void lose_malloc(){
  // Create and lose some mallocs
  void *l = malloc(sizeof(double));
  void *p = malloc(2 * sizeof(int));
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
  l1->num2 = 11;
  l1->inner = l2;
  l2->name = "inner";
  return l1;
}
//globs
void *GLOBAL_VAR_malloc;
int glob;

int main(int argc, char **argv)
{
  // inspect_allocs()
  // glob = 9 * 20;
  // GLOBAL_VAR_malloc = malloc(sizeof(double));
  // // lose_malloc();
  // struct maze *l3 = nested_mallocs();
  // another_one = l1;
  void *p = malloc(5 * sizeof(int));
  // void *chmalloc = malloc(10 * sizeof(char));
  void *ptrs[] = { main, p, &p, NULL };
  for (void **x = &ptrs[0]; *x; ++x)
  {
    printf("At %p is a %s-allocated object of size %u, type %s\n",
      *x,
      alloc_get_allocator(*x)->name,
      (unsigned) alloc_get_size(*x),
      UNIQTYPE_NAME(alloc_get_type(*x))
    );
  }
  

  free(p);

  // // /* 
  // // * Mark phase depth first search - 
  // // * iterate over root objects that contain pointers to heap storage which has been allocated to the user by malloc
  // // * and mark the chunks pointed to as 'reached'. Follow the child pointers
  // // * inside these chunks and mark them as 'reached' as well. At the end of the mark phase,
  // // * every marked object in the heap is black and every unmarked object is white.
  // // */ 
  /*
  * Collect white objects in the heap during sweep
  */
  // mark_And_sweep();

  // // scan_segments_of_executable(NULL, NULL);


  lose_malloc();
  
  // mark_And_sweep();

  // inspect_allocs();


  // for (int i = 0; i < 10000; ++i){
  //   void *p = malloc(sizeof(char));
  // }
  // inspect_allocs();

  // mark_And_sweep();

  // inspect_allocs();

  return 0;
}
