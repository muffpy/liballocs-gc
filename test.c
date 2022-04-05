#include <allocs.h>
#include <stdio.h>
#include <inttypes.h>
#include "relf.h" /* Contains fake_dladdr and _r_debug */
#include "heap_index.h"
#include "GC_funcs.h"
// #include "boehm/gc.h"

// /* ------------------ Operations on malloc'd chunks and uniqtypes ------------------*/

// typedef struct __uniqtype_node_rec_s
// { 
// 	void* obj;
// 	struct uniqtype *t;
// 	void *info;
// 	void (*free)(void *);
// 	struct __uniqtype_node_rec_s *next;
// } __uniqtype_node_rec;

// typedef void track_ptr_fn(void**, struct uniqtype**);

// /* Check if pointed to object (p_obj) is in heap-allocated storage and set mark bit 
// * if true
// */
// void __uniqtype_follow_ptr_heap(void **p_obj, struct uniqtype **p_t)
// {
//   // printf("In followptr with %p \n", *p_obj);
//   // printf("", alloc_get_allocator(*x)->name,)
//   // struct big_allocation test = __lookup_bigalloc_from_root(*p_obj);

//   struct big_allocation *arena = __lookup_bigalloc_from_root_by_suballocator(*p_obj,
// 		&__generic_malloc_allocator, NULL);
//     if (arena){ /* If found, find insert for p_obj/chunk returned to applciation by dlmalloc */
//       struct insert *cur_insert = insert_for_chunk(*p_obj);
//       printf("flag: %u, site: %lx and bits: %u \n", cur_insert->alloc_site_flag,cur_insert->alloc_site,cur_insert->un.bits);
      
//       /* Use bits field inside insert as marker */
//       unsigned bits = cur_insert->un.bits;
//       if (bits & 0x1 == 1) {printf("Mark bit already set, possibly by interior pointer to chunk. Exiting follow_ptr_fn\n"); return;} // Check mark bit isn't set. That would be strange
//       cur_insert->un.bits ^= 1; // Flip LSB
//       // printf("New bits: %u \n",cur_insert->un.bits);
//     }
// }

// void uniqtype_bfs(struct uniqtype* t, void *obj_start, unsigned start_offset, track_ptr_fn *track_fn){
//   __uniqtype_node_rec *adj_u_head = NULL;
// 	__uniqtype_node_rec *adj_u_tail = NULL;
//   build_adjacency_list_recursive(&adj_u_head, &adj_u_tail, 
// 			obj_start, t, 
// 			start_offset, t,
// 			track_fn, NULL
// 		);
// }

// struct chunk_info_table 
// {
//   void *cur_chunk;
//   void *prev_chunk;
//   struct insert *ins;
//   struct arena_bitmap_info *bitmap_info;
// };

// void inspect_uniqtype(struct uniqtype* out_type) {
//   if (out_type){
//     printf("\n");
//     printf("kind: %d \n", UNIQTYPE_KIND(out_type));
//     printf("size in bytes? : %u \n", UNIQTYPE_SIZE_IN_BYTES(out_type));
//     printf("is ptr? %d \n", UNIQTYPE_IS_POINTER_TYPE(out_type));
//     printf("array length: %u \n", UNIQTYPE_ARRAY_LENGTH(out_type));
//     // printf("array elemnt type: %s \n", UNIQTYPE_NAME(UNIQTYPE_ARRAY_ELEMENT_TYPE(out_type)));
//     printf("name: %s \n", UNIQTYPE_NAME(out_type));
//     printf("pointee type: %d \n", UNIQTYPE_POINTEE_TYPE(out_type));
//     printf("is composite type? %d \n", UNIQTYPE_IS_COMPOSITE_TYPE(out_type));
//     printf("\n");
//   }
// }

// /*
// */
// static void *view_chunk_uniqtype_metadata(struct chunk_info_table *tab, void **xarg3) 
// {
//     struct uniqtype *out_type;
//     const void *site;
//     liballocs_err_t err = extract_and_output_alloc_site_and_type(tab->ins, &out_type, &site);
//     // if (err) printf("Extraction error! \n");
//     if (!err){
//       inspect_uniqtype(out_type);
//     }
//     // return NULL;
// }

// /*
// */
// static void *find_allocated_chunk_for_userptr(struct chunk_info_table *tab, void **userptr) 
// {
//     void *cur_chunk = tab->cur_chunk;
//     void *prev_chunk = tab->prev_chunk;
//     if (tab->cur_chunk <= *userptr) return NULL; /* Continue scanning chunks */
//     else {
//       printf("Target alloca chunk at %p and p_obj at %p\n", tab->prev_chunk, *userptr);
//       return tab->prev_chunk; /* Stop scanning and return target chunk */
//     }
// }

// /* 
// * Iterates over all allocated chunks (and their insert fields) within heap memory 
// * fn_arg: function ptr which can be given allocated chunk inserts as input
// */
// void *scan_all_allocated_chunks(struct arena_bitmap_info* info, 
//     void* (*chunk_fn)(struct chunk_info_table *, void**), void **fn_arg) {
//   // pretend we're currently on 'one before the initial search start pos'
//   unsigned long cur_bit_idx = -1;
//   void *prev_usrchunk, *cur_userchunk; 
//   void *ret = NULL; /* init chunk_fn return value */
//   /* Iterate forward over bits */
//   while ((unsigned long)-1 != (cur_bit_idx = bitmap_find_first_set1_geq_l(
//                   info->bitmap, info->bitmap + info->nwords,
//                   cur_bit_idx + 1, NULL)))
//   {
//           prev_usrchunk = cur_userchunk;
//           cur_userchunk = (void*)((uintptr_t) info->bitmap_base_addr
//                   + (cur_bit_idx * MALLOC_ALIGN));
//           printf("Walking an alloca chunk at %p (bitmap array base addr: %p) idx %d\n", cur_userchunk,
//                   (void*) info->bitmap_base_addr, (int) cur_bit_idx % 64);
//           struct insert *cur_insert = insert_for_chunk(cur_userchunk);
//           printf("flag: %u, site: 0x%lx and bits: %u \n", cur_insert->alloc_site_flag,cur_insert->alloc_site, cur_insert->un.bits);
//           struct chunk_info_table tab = {
//             .cur_chunk = cur_userchunk,
//             .prev_chunk = prev_usrchunk,
//             .ins = cur_insert,
//             .bitmap_info = info
//           };

//           if (chunk_fn) {
//             ret = chunk_fn(&tab, fn_arg);
//             if (ret) return ret;
//           }
          
//           // view_chunk_uniqtype_metadata(&tab, fn_arg);
//   }
//   return NULL;
// }

// /* For sanity checking
// */
// void inspect_allocs() {
//   printf("\nInspecting allocs --- \n");
//   for (int i = 0; i < NBIGALLOCS; ++i) {
//     // printf("Big alloc entry %i starts at %p ends at %p\n", i, big_allocations[i].begin, big_allocations[i].end);
//     if (big_allocations[i].suballocator == &__generic_malloc_allocator)
//     {
//       printf("Big alloc entry %i starts at %p ends at %p\n", i, big_allocations[i].begin, big_allocations[i].end);
//       struct big_allocation *arena = &big_allocations[i];
//       struct arena_bitmap_info *info = arena->suballocator_private;
//       bitmap_word_t *bitmap = info->bitmap;

//       printf("Bitmap: %"PRIxPTR"\n", bitmap);
//       for (int i= 0; i < info->nwords; ++i) {
//         if (info->bitmap[i]){
//           printf("Word at position %i is %lx \n", i, info->bitmap[i]);
//         }
//       }
//       scan_all_allocated_chunks(info,NULL,NULL);
//     }
//   }
//   printf("Finished inspection --- \n");
// }

// /* -------------------------- Stack-walking helpers ----------------------- */

// /*  Gets uniqtype of frame and explores subobjects inside it. Calls follow_ptr fn on
//  pointers found during search
// */
// static int mark_cb(void *ip, void *sp, void *bp, track_ptr_fn *track_fn)
// {
//   // Get type information for each frame
//   struct frame_uniqtype_and_offset s = pc_to_frame_uniqtype(ip);
//   struct uniqtype *frame_desc = s.u;
//   printf ("ip = %p, bp = %p, sp = %p\n", ip, bp, sp);
// 		// What's the frame *allocation* base? It's the frame_base *minus*
// 		// the amount that s told us. 
// 	unsigned char *frame_allocation_base = (unsigned char *) bp - s.o; 
//   if (frame_desc){
//     uniqtype_bfs(frame_desc, frame_allocation_base, 0, track_fn);
//     printf("frame kind: %d \n", UNIQTYPE_KIND(frame_desc));
//   }

// 	return 0; // keep going
// }

// static int liballocs_unwind_stack(int (*cb)(void *, void *, void *, void *), track_ptr_fn *xarg)
// {
//   printf("Walking the stack ... \n");
// 	unw_cursor_t cursor; unw_context_t unw_context;
// 	unw_word_t nextframe_sp = 0, nextframe_bp = 0, nextframe_ip = 0, sp, bp = 0, ip = 0;
// 	int unw_ret;
// 	int ret = 0;
	
// 	/* Get an initial snapshot. */
// 	unw_ret = unw_getcontext(&unw_context);
//   /* Initialize an unwind cursor based on this snapshot. */
// 	unw_init_local(&cursor, &unw_context);

// 	unw_get_reg(&cursor, UNW_REG_SP, &nextframe_sp);
//   unw_word_t check_sp;
// #ifdef UNW_TARGET_X86
// 	__asm__ ("movl %%esp, %0\n" :"=r"(check_sp));
// #else
// 	__asm__("movq %%rsp, %0\n" : "=r"(check_sp)); // X86_64
// #endif
//   assert(check_sp == nextframe_sp);

// #define HIGHEST_STACK_ADDRESS ((uintptr_t) MAXIMUM_USER_ADDRESS)
// 	while(nextframe_sp != HIGHEST_STACK_ADDRESS) // innermost frame
// 	{
// 		/* Get ip, bp and sp of current stack frame. */
// 		unw_get_reg(&cursor, UNW_REG_IP, &ip);
// 		unw_get_reg(&cursor, UNW_REG_SP, &sp);
// 		/* Try to get the bp, but may fail (due to compiler optimization) */
// 		unw_ret = unw_get_reg(&cursor, UNW_TDEP_BP, &bp); _Bool have_bp = (unw_ret == 0);

//     /* Get symname of current procedure */
// 		char sym[256]; unw_word_t offset;
// 		printf("0x%lx:", ip);
// 		if (unw_get_proc_name(&cursor, sym, sizeof(sym), &offset) == 0) {
// 			printf(" (%s+0x%lx)\n", sym, offset);
// 		} else {
// 			printf(" -- error: unable to obtain symbol name for this frame\n");
// 		}

//     /* Move unwind cursor toward beginning of stack (higher addresses) */
// 		int unw_step_ret = unw_step(&cursor);
// 		if (unw_step_ret > 0)
// 		{
//       /* No problem if we don't have bp, just use nextframe_sp */
// 			unw_get_reg(&cursor, UNW_REG_SP, &nextframe_sp); 
//       bp = have_bp ? bp : nextframe_sp;

//       ret = cb((void*) ip, (void*) sp, (void*) bp, xarg);
// 		  if (ret) return ret;
// 		}
// 		else if (unw_step_ret == 0) /* Final frame reached */
// 		{
// 			nextframe_sp = HIGHEST_STACK_ADDRESS;
// 		}
// 		else /* Unwinding failure */
// 		{
// 			ret = -1;
// 			break;
// 		}
// 	}
// 	return ret;
// }

// /* --------------------- Static allocation handlers  ------------------- */

// static void* metavector_follow_ptr_to_heap(void *obj_start, struct uniqtype* obj_type, void** xarg){
//     const char* kind = obj_type->un.info.kind;
//       if ( kind != 0 && kind != 0x2 && kind != 0xa && kind != 0xc ) { 
//         /* If not void, base, subprogram or subrange type, then continue. See DWARF debug format information 
//         at https://developer.ibm.com/articles/au-dwarf-debug-format/ */
        
//         if (kind == 0x8) { /* is an address */
//           void *ptr_to_malloc = *(void**)(obj_start);
//           printf("Ptr to malloc: %p \n", ptr_to_malloc);

//           struct big_allocation* big = __lookup_bigalloc_top_level(obj_start);
//           printf("Top alloc entry starts at %p ends at %p\n", big->begin, big->end);
//           struct big_allocation* deep = __lookup_deepest_bigalloc(obj_start);
//           printf("Deep alloc entry starts at %p ends at %p\n", deep->begin, deep->end);
//           struct allocator *leaf_alloc = __liballocs_leaf_allocator_for(ptr_to_malloc, &deep);
//           printf("Leaf alloc: %s \n", leaf_alloc->name);

//           /* Check if address points to a heap object */
//           if (leaf_alloc == &__generic_malloc_allocator){ 
//             struct uniqtype *pointed_to_static_t = UNIQTYPE_POINTEE_TYPE(obj_type);
//             /* Mark this object */
//             __uniqtype_follow_ptr_heap(obj_start, &pointed_to_static_t);
//           }
//         }
//         else { /* If we have a thing with a structure, recurse through it*/
//           uniqtype_bfs(obj_type, obj_start, 0, __uniqtype_follow_ptr_heap);
//           printf("\n");
//         }
//       }
//         // struct insert *cur_insert = insert_for_chunk(x);
//         // printf("flag: %u, site: %lx and bits: %u \n", cur_insert->alloc_site_flag,cur_insert->alloc_site,cur_insert->un.bits);
//   return NULL;
// }

// /*
// */
// void scan_segments_of_executable(void* (*metavector_fn)(void*, struct uniqtype *, void**), void** xarg)
// {
//     printf("Walking the link map of DSOs ... \n");
//     for (struct link_map *l = _r_debug.r_map; l; l = l->l_next)
//     {
//       /* l_addr isn't guaranteed to be mapped, so use _DYNAMIC a.k.a. l_ld'*/
//       void *query_addr = l->l_ld;
//       printf("New DSO %s at %p \n", l->l_name, query_addr);
//       if (query_addr > (void *) 0x7f0000000000) continue; // Temporary hack
//       struct big_allocation *containing_mapping =__lookup_bigalloc_top_level(query_addr);
//       struct big_allocation *containing_file = __lookup_bigalloc_under(
//         query_addr, &__static_file_allocator, containing_mapping, NULL);
//       assert(containing_file);
//       struct allocs_file_metadata *afile =
//             containing_file->allocator_private;
//       for (unsigned i_seg = 0; i_seg < afile->m.nload; ++i_seg)
//       {
//         union sym_or_reloc_rec *metavector = afile->m.segments[i_seg].metavector;
//         size_t metavector_size = afile->m.segments[i_seg].metavector_size;
//         // we print the whole metavector
//           for (unsigned i = 0; i < metavector_size / sizeof *metavector; ++i)
//           {
//             struct uniqtype *t = NULL;
//             uintptr_t base_vaddr = vaddr_from_rec(&metavector[i], afile);
//             if (!metavector[i].is_reloc) {
//               t = (struct uniqtype *)(((uintptr_t) metavector[i].sym.uniqtype_ptr_bits_no_lowbits)<<3);
//             }
//             printf("At %016lx there is a static alloc of kind %u, idx %08u, type %s\n",
//               afile->m.l->l_addr + base_vaddr,
//               (unsigned) (metavector[i].is_reloc ? REC_RELOC : metavector[i].sym.kind),
//               (unsigned) (metavector[i].is_reloc ? 0 : metavector[i].sym.idx),
//               UNIQTYPE_NAME(t)
//             );

//             void *x = (afile->m.l->l_addr + base_vaddr);
//             struct uniqtype *xtype = alloc_get_type(x);
//             struct allocator *xalloc = alloc_get_allocator(x);
//             unsigned long xsz = alloc_get_size(x); 
//             printf("At %p is a %s-allocated object of size %u, type %s, composite type? %lx and kind %d\n\n",
//                     x,
//                     xalloc ? xalloc->name : "no alloc found",
//                     xsz,
//                     xtype ? UNIQTYPE_NAME(xtype) : "no type found",
//                     xtype ? UNIQTYPE_IS_COMPOSITE_TYPE(xtype) : 0U,
//                     xtype ? UNIQTYPE_KIND(xtype) : 0
//                   );
//             if (metavector_fn && xtype) {
//               void* ret = NULL;
//               ret = metavector_fn(x, xtype, xarg);
//               // if (ret) return ret /* What to do with ret? */
//             }
//           }
//       }
//       printf("\n");
//     }
// }


// /* ------------------------------- Mark-and-sweep ----------------------------- */

// #define RECENTLY_ALLOCATED_ARENAS_LIST_SIZE 20
// static void *recently_allocated_arenas[RECENTLY_ALLOCATED_ARENAS_LIST_SIZE];

// void mark_stack_and_register_roots() {
//   printf("\n");
//   printf("Marking allocs from stack frames and registers ... \n");
//   int ret = liballocs_unwind_stack(mark_cb, __uniqtype_follow_ptr_heap);
//   printf("\n");
// }

// void mark_static_allocs() {
//   printf("\n");
//   printf("Marking static allocs ... \n");
//   scan_segments_of_executable(metavector_follow_ptr_to_heap, NULL);
// }

// /* Removes mark bit from a black object and frees white objects
// */
// void *sweep_garbage(struct chunk_info_table *tab, void **arena_ptr){
//   // printf("Insert in sweep(): flag: %u, site: 0x%lx and bits: %u \n", tab->ins->alloc_site_flag,tab->ins->alloc_site, tab->ins->un.bits);
//   if (tab->ins->un.bits & 0x1) /* If marked, unmark*/ {tab->ins->un.bits ^= 1;}
//   // else if (tab->ins->alloc_site_flag) { } /* Ignore inserts that are types*/
//   else { /* Free the unreachable garbage */
//     struct big_allocation *arena = *arena_ptr;
//     printf("*** Deleting entry for chunk %p, from bitmap at %p\n", 
// 		tab->cur_chunk);
//     __liballocs_bitmap_delete(arena, tab->cur_chunk);
//   }
//   return NULL; /* Continue collection */
// }

// void sweep() 
// {
//   printf("\nSweeping allocs --- \n");
//   for (int i = 0; i < NBIGALLOCS; ++i) {
//     // printf("Big alloc entry %i starts at %p ends at %p\n", i, big_allocations[i].begin, big_allocations[i].end);
//     if (big_allocations[i].suballocator == &__generic_malloc_allocator)
//     {
//       printf("Big alloc entry %i starts at %p ends at %p\n", i, big_allocations[i].begin, big_allocations[i].end);
//       struct big_allocation *arena = &big_allocations[i];
//       struct arena_bitmap_info *info = arena->suballocator_private;
//       bitmap_word_t *bitmap = info->bitmap;

//       printf("Bitmap: %"PRIxPTR"\n", bitmap);
//       for (int i= 0; i < info->nwords; ++i) {
//         if (info->bitmap[i]){
//           printf("Word at position %i is %lx \n", i, info->bitmap[i]);
//         }
//       }
//       printf("nwords: %lu \n", info->nwords);
//       scan_all_allocated_chunks(info,sweep_garbage,&arena);
//     }
//   }
//   printf("Finished sweep --- \n");
// }

// #define mark_And_sweep() {mark_stack_and_register_roots();mark_static_allocs();sweep();}

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


  for (int i = 0; i < 10000; ++i){
    void *p = malloc(sizeof(char));
  }

  // inspect_allocs();


  // mark_And_sweep();

  // inspect_allocs();

  return 0;
}
