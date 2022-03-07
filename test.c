#include <allocs.h>
#include <stdio.h>
#include <inttypes.h>
#include "relf.h" // Contains fake_dladdr and _r_debug
#include "heap_index.h"

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

    for (struct link_map *l = _r_debug.r_map; l; l = l->l_next)
    {
      /* l_addr isn't guaranteed to be mapped, so use _DYNAMIC a.k.a. l_ld'*/
      void *query_addr = l->l_ld;
      struct big_allocation *containing_mapping =__lookup_bigalloc_top_level(query_addr);
      struct big_allocation *containing_file = __lookup_bigalloc_under(
        query_addr, &__static_file_allocator, containing_mapping, NULL);
      assert(containing_file);
      struct allocs_file_metadata *afile =
            containing_file->allocator_private;
      for (unsigned i_seg = 0; i_seg < afile->m.nload; ++i_seg)
      {
        union sym_or_reloc_rec *metavector = afile->m.segments[i_seg].metavector;
        size_t metavector_size = afile->m.segments[i_seg].metavector_size;
#if 1 /* librunt doesn't do this */
        // we print the whole metavector
        if (__liballocs_debug_level >= 10){ 
          for (unsigned i = 0; i < metavector_size / sizeof *metavector; ++i)
          {
            fprintf("At %016lx there is a static alloc of kind %u, idx %08u, type %s\n",
              afile->m.l->l_addr + vaddr_from_rec(&metavector[i], afile),
              (unsigned) (metavector[i].is_reloc ? REC_RELOC : metavector[i].sym.kind),
              (unsigned) (metavector[i].is_reloc ? 0 : metavector[i].sym.idx),
              UNIQTYPE_NAME(
                metavector[i].is_reloc ? NULL :
                (struct uniqtype *)(((uintptr_t) metavector[i].sym.uniqtype_ptr_bits_no_lowbits)<<3)
              )
            );
          }
        }
#endif
      }
    }
}

static void walk_bitmap(struct arena_bitmap_info* info) {
  // pretend we're currently on 'one before the initial search start pos'
  unsigned long cur_bit_idx = 0;
  
  /* Iterate forward over bits */
  unsigned long total_unindexed = 0;
  while ((unsigned long)-1 != (cur_bit_idx = bitmap_find_first_set1_geq_l(
                  info->bitmap, info->bitmap + info->nwords,
                  cur_bit_idx + 1, NULL)))
  {
          void *cur_userchunk = (void*)((uintptr_t) info->bitmap_base_addr
                  + (cur_bit_idx * ALLOCA_ALIGN));
          printf("Walking an alloca chunk at %p (bitmap base: %p) idx %d\n", cur_userchunk,
                  (void*) info->bitmap_base_addr, (int) cur_bit_idx);
          struct insert *cur_insert = insert_for_chunk(cur_userchunk);
          printf("flag: %u, site: %lu \n", cur_insert->alloc_site_flag, cur_insert->alloc_site);
          // unsigned long bytes_to_unindex = malloc_usable_size(cur_userchunk);
          // assert(ALLOCA_ALIGN == MALLOC_ALIGN);
          // equal alignments so we can just abuse the generic malloc functions
          // __liballocs_bitmap_delete(b, cur_userchunk);
          // assert(bytes_to_unindex < BIGGEST_SANE_ALLOCA);
          // total_unindexed += bytes_to_unindex;
          // if (total_unindexed >= total_to_unindex)
          // {
          //         if (total_unindexed > total_to_unindex)
          //         {
          //                 fprintf(stderr, 
          //                         "Warning: unindexed too many bytes "
          //                         "(requested %lu from %p; got %lu)\n",
          //                         total_to_unindex, frame_addr, total_unindexed);
          //         }
          //         goto out;
          // }
  }
}

void *GLOBAL_VAR_malloc;
int glob;

int main(int argc, char **argv)
{ 
  glob = 4 * 20;
  GLOBAL_VAR_malloc = malloc(sizeof(int));
  void *p = malloc(42 * sizeof(int));
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

  struct big_allocation *arena = __lookup_bigalloc_from_root_by_suballocator(GLOBAL_VAR_malloc,
		&__generic_malloc_allocator, NULL);

  printf("big alloc begins at %p\n", arena->begin);
  struct arena_bitmap_info *info = arena->suballocator_private;
  bitmap_word_t *bitmap = info->bitmap;

  printf("Bitmap: %"PRIxPTR"\n", bitmap);
  printf("nwords: %lu \n", info->nwords);
  walk_bitmap(info);
  return 0;
}
