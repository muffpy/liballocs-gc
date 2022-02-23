#include <allocs.h>
#include <stdio.h>
#include <inttypes.h>
#include <pageindex.h>
#include "relf.h" // Contains fake_dladdr and _r_debug
#include <allocmeta.h>

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

void debug_segments(void)
{
	if (__liballocs_debug_level >= 10)
		{
			for (struct link_map *l = _r_debug.r_map; l; l = l->l_next)
			{
				/* l_addr isn't guaranteed to be mapped, so use _DYNAMIC a.k.a. l_ld'*/
				void *query_addr = l->l_ld;
				struct big_allocation *containing_mapping =__lookup_bigalloc_top_level(query_addr);
				struct big_allocation *containing_file = __lookup_bigalloc_under(
					query_addr, &__static_file_allocator, containing_mapping, NULL);
				assert(containing_file);
				struct allocs_file_metadata *afile =
						 containing_file->meta.un.opaque_data.data_ptr;	
				for (unsigned i_seg = 0; i_seg < afile->m.nload; ++i_seg)
				{
					union sym_or_reloc_rec *metavector = afile->m.segments[i_seg].metavector;
					size_t metavector_size = afile->m.segments[i_seg].metavector_size;
#if 1 /* librunt doesn't do this */
					// we print the whole metavector
					for (unsigned i = 0; i < metavector_size / sizeof *metavector; ++i)
					{
						fprintf(get_stream_err(), "At %016lx there is a static alloc of kind %u, idx %08u, type %s\n",
							afile->m.l->l_addr + vaddr_from_rec(&metavector[i], afile),
							(unsigned) (metavector[i].is_reloc ? REC_RELOC : metavector[i].sym.kind),
							(unsigned) (metavector[i].is_reloc ? 0 : metavector[i].sym.idx),
							UNIQTYPE_NAME(
								metavector[i].is_reloc ? NULL :
								(struct uniqtype *)(((uintptr_t) metavector[i].sym.uniqtype_ptr_bits_no_lowbits)<<3)
							)
						);
					}
#endif
				}
			}
		}
}

// void *GLOBAL_VAR_malloc;

int main(int argc, char **argv)
{
  // GLOBAL_VAR_malloc = malloc(sizeof(int));
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

  __liballocs_walk_stack(callback, NULL);

  // Print out read/write segment metadata
//   debug_static_file_allocator();
   debug_segments();

  return 0;
}
