#include <allocs.h>
#include <stdio.h>
#include <inttypes.h>

// Contains fake_dladdr
#include "/usr/local/src/liballocs/contrib/libsystrap/contrib/librunt/include/relf.h"

/*
* Params: ip (instruction ptr) : next instruction to be executed
          sp (stack ptr) : top of the current frame used in the stack of current thread
          bp (base ptr) : top of previous stack frame ?
  
  The function fake_dladdr(addr, ...args) determines whether the address specified in
  addr is located in one of the shared objects loaded by the
  calling application.  If it is, then fake_dladdr() returns information
  about the shared object (fname and fbase) and symbol (sname and saddr) that overlaps addr.
*/
static int callback(void *ip, void *sp, void *bp, void *arg)
{
	const char *sname = ip ? "(unknown)" : "(no active function)";
	int ret = ip ? fake_dladdr(ip, NULL, NULL, &sname, NULL) : 0;
	printf("%s\n", sname);
	return 0; // keep going
}

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
  // free(p);

  // Retrive sp
  void *sp;
  __asm__("movq %%rsp, %0\n" : "=r"(sp));
  printf("%p\n", sp);

  // Get liballocs metadata
  printf("%hi\n",PAGENUM(sp));
  printf("%p\n", pageindex);
  printf("%u\n", pageindex[PAGENUM(sp)]);

  struct big_allocation *st;
  st = &big_allocations[pageindex[PAGENUM(sp)]];

  printf("big alloc begins at %p\n", st->begin);

  // #define USE_FAKE_LIBUNWIND = true
  __liballocs_walk_stack(callback, NULL);
  return 0;
}
