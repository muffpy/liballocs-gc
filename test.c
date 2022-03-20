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
  addr is located in one of the shared objects loaded by the calling application.  
  If it is, then fake_dladdr() returns information about the shared object (fname and fbase) 
  and symbol (sname and saddr) that overlaps addr.
*/
static int st_callback(void *ip, void *sp, void *bp, void *arg)
{
	// const char *sname = ip ? "(unknown)" : "(no active function)";
	// int ret = ip ? fake_dladdr(ip, NULL, NULL, &sname, NULL) : 0;
	// printf("%s\n", sname);

  // Get type information for each frame
  struct frame_uniqtype_and_offset s = pc_to_frame_uniqtype(ip);
  struct uniqtype *frame_desc = s.u;
  if (frame_desc){
    printf("frame kind: %d \n", UNIQTYPE_KIND(frame_desc));
  }
  printf ("ip = %p, bp = %p, sp = %p\n", ip, bp, sp);
	return 0; // keep going
}

void view_segments(void)
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

static void *find_allocated_chunks(struct arena_bitmap_info* info, void *mem) {
  // pretend we're currently on 'one before the initial search start pos'
  unsigned long cur_bit_idx = -1;
  
  /* Iterate forward over bits */
  while ((unsigned long)-1 != (cur_bit_idx = bitmap_find_first_set1_geq_l(
                  info->bitmap, info->bitmap + info->nwords,
                  cur_bit_idx + 1, NULL)))
  {
          void *cur_userchunk = (void*)((uintptr_t) info->bitmap_base_addr
                  + (cur_bit_idx * MALLOC_ALIGN));
          printf("Walking an alloca chunk at %p (bitmap base: %p) idx %d\n", cur_userchunk,
                  (void*) info->bitmap_base_addr, (int) cur_bit_idx);
          struct insert *cur_insert = insert_for_chunk(cur_userchunk);
          printf("flag: %u, site: %lu \n", cur_insert->alloc_site_flag,cur_insert->alloc_site);
          struct uniqtype *out_type;
          const void *site;
          liballocs_err_t err = extract_and_output_alloc_site_and_type(cur_insert, &out_type, &site);
          printf("\n");
          // if (!out_type) {
          //   out_type = __uniqtype__ARR___uninterpreted_byte;
          // }
          printf("site ptr %p \n", site);
          printf("kind: %d \n", UNIQTYPE_KIND(out_type));
          printf("size in bytes? : %u \n", UNIQTYPE_SIZE_IN_BYTES(out_type));
          printf("is ptr? %d \n", UNIQTYPE_IS_POINTER_TYPE(out_type));
          printf("array length: %u \n", UNIQTYPE_ARRAY_LENGTH(out_type));
          // printf("array elemnt type: %s \n", UNIQTYPE_NAME(UNIQTYPE_ARRAY_ELEMENT_TYPE(out_type)));
          printf("name: %s \n", UNIQTYPE_NAME(out_type));
          printf("pointee type: %d \n", UNIQTYPE_POINTEE_TYPE(out_type));
          printf("is subprogram type? %d \n", UNIQTYPE_IS_SUBPROGRAM_TYPE(out_type));
          printf("\n");
          // print(out_type->)
  }
}

void *GLOBAL_VAR_malloc;
int glob;

void lose_malloc(){
  void *l = malloc(sizeof(double));
}

struct maze {
  int num;
  char *name;
};
struct maze *nested_mallocs() {
  struct maze *l1;
  l1 = malloc(sizeof(struct maze));
  l1->num = 1;
  l1->name = malloc(20);

  return l1;
}

void mark() {
  for (int i = 0; i < NBIGALLOCS; ++i) {
    if (big_allocations[i].suballocator == &__generic_malloc_allocator
			|| big_allocations[i].suballocator == &__alloca_allocator)
    {
      printf("Big alloc entry %i starts at %p ends at %p\n", i, big_allocations[i].begin, big_allocations[i].end);
      struct big_allocation *arena = &big_allocations[i];
      struct arena_bitmap_info *info = arena->suballocator_private;
      bitmap_word_t *bitmap = info->bitmap;

      printf("Bitmap: %"PRIxPTR"\n", bitmap);
      // for (int i= 0; i < info->nwords; ++i) {
      //   printf("Word at position %i is %lx \n", i, info->bitmap[i]);
      // }
      printf("nwords: %lu \n", info->nwords);
      void *chunk = find_allocated_chunks(info, arena->begin);
    }
  }
}

int main(int argc, char **argv)
{ 
  glob = 4 * 20;
  // GLOBAL_VAR_malloc = malloc(sizeof(int));
  lose_malloc();
  struct maze *l1 = nested_mallocs();
  void *p = malloc(5 * sizeof(int));
  void *chmalloc = malloc(10 * sizeof(char));
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

  // printf("big alloc begins at %p\n", st->begin);
  
  __liballocs_walk_stack(st_callback, NULL);
  printf("\n");

  // Print out read/write segment metadata
 //   debug_static_file_allocator();
  view_segments();

  /* 
  * Mark phase depth first search - 
  * iterate over heap arenas that contain chunks which have been allocated to the user by malloc
  * and mark these chunks as 'reached' using the mark function. Follow the child pointers
  * inside these chunks and mark them as 'reached' as well. At the end of the mark phase,
  * every marked object in the heap is black and every unmarked object is white.
  */ 
  // mark(); 

  /*
  * Collect white objects in the heap
  */


  // proof check
  // struct big_allocation *arena1 = __lookup_bigalloc_from_root_by_suballocator(GLOBAL_VAR_malloc,
	// 	&__generic_malloc_allocator, NULL);
  // struct big_allocation *arena2 = __lookup_bigalloc_from_root_by_suballocator(p,
	// 	&__generic_malloc_allocator, NULL);

  // printf("\n\narena1 alloc begins at %p\n", arena1->begin);
  // printf("arena1 alloc   ends at %p\n", arena1->end);
  // printf("arena2 alloc begins at %p\n", arena2->begin);
  // printf("arena2 alloc   ends at %p\n", arena2->end);
  // struct arena_bitmap_info *info = arena->suballocator_private;
  // bitmap_word_t *bitmap = info->bitmap;

  // printf("Bitmap: %"PRIxPTR"\n", bitmap);
  // printf("nwords: %lu \n", info->nwords);
  // find_chunk(info, arena->begin);

  return 0;
}
