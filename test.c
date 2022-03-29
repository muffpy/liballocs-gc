#include <allocs.h>
#include <stdio.h>
#include <inttypes.h>
#include "relf.h" /* Contains fake_dladdr and _r_debug */
#include "heap_index.h"
#include "GC_funcs.h"
// #include "boehm/gc.h"

/* --------------------- Static allocation handlers  ------------------- */
void mark_segments()
{
    for (struct link_map *l = _r_debug.r_map; l; l = l->l_next)
    {
      /* l_addr isn't guaranteed to be mapped, so use _DYNAMIC a.k.a. l_ld'*/
      void *query_addr = l->l_ld;
      printf("New DSO %s at %p \n", l->l_name, query_addr);
      if (query_addr > (void *) 0x7f0000000000) continue; // Temporary hack
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
        // if (__liballocs_debug_level >= 10){ 
          for (unsigned i = 0; i < metavector_size / sizeof *metavector; ++i)
          {
            struct uniqtype *sym = NULL;
            uintptr_t base_vaddr = vaddr_from_rec(&metavector[i], afile);
            if (!metavector[i].is_reloc) {
              sym = (struct uniqtype *)(((uintptr_t) metavector[i].sym.uniqtype_ptr_bits_no_lowbits)<<3);
              // uniqtype_bfs(sym, afile->m.l->l_addr + vaddr_from_rec(&metavector[i], afile), 0);
            }
            printf("At %016lx there is a static alloc of kind %u, idx %08u, type %s\n",
              afile->m.l->l_addr + base_vaddr,
              (unsigned) (metavector[i].is_reloc ? REC_RELOC : metavector[i].sym.kind),
              (unsigned) (metavector[i].is_reloc ? 0 : metavector[i].sym.idx),
              UNIQTYPE_NAME(sym)
            );

            void *x = (void*)(afile->m.l->l_addr + base_vaddr);
            struct uniqtype *xtype = alloc_get_type(x);
            struct allocator *xalloc = alloc_get_allocator(x);
            unsigned long xsz = alloc_get_size(x); 
            printf("At %p is a %s-allocated object of size %u, type %s\n\n",
                    x,
                    xalloc ? xalloc->name : "no alloc found",
                    xsz,
                    xtype ? UNIQTYPE_NAME(xtype) : "no type found"
                  );
          } 
        // } 
#endif
      }
      printf("\n");
    }
}

/* --------------------- Stack-walking helpers and callbacks ------------------- */

typedef struct __uniqtype_node_rec_s
{ 
	void* obj;
	struct uniqtype *t;
	void *info;
	void (*free)(void *);
	struct __uniqtype_node_rec_s *next;
} __uniqtype_node_rec;

typedef void track_ptr_fn(void**, struct uniqtype**);

void uniqtype_bfs(struct uniqtype* t, void *obj_start, unsigned start_offset, track_ptr_fn *track_fn){
  __uniqtype_node_rec *adj_u_head = NULL;
	__uniqtype_node_rec *adj_u_tail = NULL;
  build_adjacency_list_recursive(&adj_u_head, &adj_u_tail, 
			obj_start, t, 
			start_offset, t,
			track_fn, NULL
		);
}

/*  Gets uniqtype of frame and explores subobjects inside it. Calls follow_ptr fn on
 pointers found during search
*/
static int mark_cb(void *ip, void *sp, void *bp, track_ptr_fn *track_fn)
{
  // Get type information for each frame
  struct frame_uniqtype_and_offset s = pc_to_frame_uniqtype(ip);
  struct uniqtype *frame_desc = s.u;
  printf ("ip = %p, bp = %p, sp = %p\n", ip, bp, sp);
		// 2a. what's the frame *allocation* base? It's the frame_base *minus*
		// the amount that s told us. 
	unsigned char *frame_allocation_base = (unsigned char *) bp - s.o; 
  if (frame_desc){
    uniqtype_bfs(frame_desc, frame_allocation_base, 0, track_fn);
    printf("frame kind: %d \n", UNIQTYPE_KIND(frame_desc));
  }
	return 0; // keep going
}

/* Check if pointed to object (p_obj) is in heap-allocated storage and set mark bit 
* if true
*/
void __uniqtype_default_follow_ptr(void **p_obj, struct uniqtype **p_t)
{
  
  printf("In followptr with %p \n", *p_obj);
  // printf("", alloc_get_allocator(*x)->name,)
  // struct big_allocation test = __lookup_bigalloc_from_root(*p_obj);

  struct big_allocation *arena1 = __lookup_bigalloc_from_root_by_suballocator(*p_obj,
		&__generic_malloc_allocator, NULL);
    if (arena1){
      printf("arena1 alloc begins at %p\n", arena1->begin);
      printf("arena1 alloc   ends at %p\n", arena1->end);
    }
}
static int liballocs_unwind_stack(int (*cb)(void *, void *, void *, void *), /* Track ptr cb*/ track_ptr_fn *xarg)
{
	unw_cursor_t cursor; unw_context_t unw_context;
	unw_word_t nextframe_sp = 0, nextframe_bp = 0, nextframe_ip = 0, sp, bp = 0, ip = 0;
	int unw_ret;
	int ret = 0;
	
	/* Get an initial snapshot. */
	unw_ret = unw_getcontext(&unw_context);
  /* Initialize an unwind cursor based on this snapshot. */
	unw_init_local(&cursor, &unw_context);

	unw_get_reg(&cursor, UNW_REG_SP, &nextframe_sp);
  unw_word_t check_sp;
#ifdef UNW_TARGET_X86
	__asm__ ("movl %%esp, %0\n" :"=r"(check_sp));
#else
	__asm__("movq %%rsp, %0\n" : "=r"(check_sp)); // X86_64
#endif
  assert(check_sp == nextframe_sp);

#define HIGHEST_STACK_ADDRESS ((uintptr_t) MAXIMUM_USER_ADDRESS)
	while(nextframe_sp != HIGHEST_STACK_ADDRESS)
	{
		/* Get ip, bp and sp of current stack frame. */
		unw_get_reg(&cursor, UNW_REG_IP, &ip);
		unw_get_reg(&cursor, UNW_REG_SP, &sp);
		/* Try to get the bp, but may fail (due to compiler optimization) */
		unw_ret = unw_get_reg(&cursor, UNW_TDEP_BP, &bp); _Bool have_bp = (unw_ret == 0);

    /* Get symname of current procedure */
		char sym[256]; unw_word_t offset;
		printf("0x%lx:", ip);
		if (unw_get_proc_name(&cursor, sym, sizeof(sym), &offset) == 0) {
			printf(" (%s+0x%lx)\n", sym, offset);
		} else {
			printf(" -- error: unable to obtain symbol name for this frame\n");
		}

    /* Move unwind cursor toward beginning of stack (higher addresses) */
		int unw_step_ret = unw_step(&cursor);
		if (unw_step_ret > 0)
		{
      /* No problem if we don't have bp, just use nextframe_sp */
			unw_get_reg(&cursor, UNW_REG_SP, &nextframe_sp); 
      bp = have_bp ? bp : nextframe_sp;

      ret = cb((void*) ip, (void*) sp, (void*) bp, xarg);
		  if (ret) return ret;
		}
		else if (unw_step_ret == 0) /* Final frame reached */
		{
			nextframe_sp = HIGHEST_STACK_ADDRESS;
		}
		else /* Unwinding failure */
		{
			ret = -1;
			break;
		}
	}

	return ret;
}

void mark_stack_and_register_roots() {
  printf("\n");
  printf("Walking the stack ... \n");
  int ret = liballocs_unwind_stack(mark_cb, __uniqtype_default_follow_ptr);
  printf("\n");
}

/* ----------------------------- Extra helpers ---------------------------*/

/*
*/
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
          // if (err) printf("Extraction error! \n");
          if (!err && out_type){
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
}

/* ---------------------- Mark-and-sweep ----------------------- */
void mark()
{

}
void sweep() 
{
  for (int i = 0; i < NBIGALLOCS; ++i) {
    // printf("Big alloc entry %i starts at %p ends at %p\n", i, big_allocations[i].begin, big_allocations[i].end);
    if (big_allocations[i].suballocator == &__generic_malloc_allocator)
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
      find_allocated_chunks(info, arena->begin);
    }
  }
}

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
  l1->num2 = 6;
  l1->inner = l2;
  l2->name = "inner";
  return l1;
}
//globs
void *GLOBAL_VAR_malloc;
int glob;

int main(int argc, char **argv)
{ 
  glob = 6 * 20;
  GLOBAL_VAR_malloc = malloc(sizeof(int));
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
  free(p);

  // // /* 
  // // * Mark phase depth first search - 
  // // * iterate over root objects that contain pointers to heap storage which has been allocated to the user by malloc
  // // * and mark the chunks pointed to as 'reached'. Follow the child pointers
  // // * inside these chunks and mark them as 'reached' as well. At the end of the mark phase,
  // // * every marked object in the heap is black and every unmarked object is white.
  // // */ 
  mark_stack_and_register_roots();
  mark_segments();

  /*
  * Collect white objects in the heap
  */
  sweep();

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
