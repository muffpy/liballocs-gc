#include <allocs.h>
#include <stdio.h>
#include "/usr/local/src/liballocs/contrib/libsystrap/contrib/librunt/include/vas.h"
#include <inttypes.h>

int main(int argc, char **argv)
{
  void *p = malloc(42 * sizeof(int));
  void *ptrs[] = { main, p, &p, argv, NULL };
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

  // Retrive sp
  register void *sp asm ("rsp");
  printf("%p\n", sp);

  // Get liballocs metadata
  printf("%u\n",PAGENUM(sp));

  printf("%p\n", pageindex);
  // int n = sizeof(big_allocations[0]);
  // printf("%i\n", n);

  // for (int i=0; i<n; i++ ){
  //   printf("%" PRIu16 "\n", pageindex[i]);
  // }
  
  // printf("%u", pageindex[PAGENUM(sp)]);
  // struct big_allocation *st;
  // st = &big_allocations[pageindex[PAGENUM(sp)]];

  // printf("big alloc begins at %p\n", st->begin);
  return 0;
}
