#include <allocs.h>
#include <stdio.h>
#include <errno.h>
#include "dlmalloc.h"
#include "GC_funcs.h"
#include <inttypes.h>
#include "relf.h" /* Contains fake_dladdr and _r_debug */
#include "heap_index.h" /* struct arena_abitmap_info, struct insert & more*/

/* --- debugging --- */
#ifndef DEBUG_TEST
#define DEBUG_TEST 0
#endif

#define debug_printf(fmt, ...) \
            do { if (DEBUG_TEST) fprintf(stdout, fmt, ##__VA_ARGS__); } while (0)

/* ------------ useful dlmalloc state and macros ----------- */
#define chunkfrommem(p) ((void*)((char*)(p) + (sizeof(size_t) << 1)))
#define ok_address(M, a) (((char*)(a) - (sizeof(size_t) << 1)) >= (M).least_addr) /* Check address not lower than least address obtained from MORECORE */
extern struct malloc_state _gm_;


/* ------------------ Operations on malloc'd chunks and uniqtypes ------------------*/

/* Adapted from https://github.com/stephenrkell/liballocs/blob/master/src/uniqtype-bfs.c*/
#define IS_PLAUSIBLE_POINTER(p) ((p) && ((p) != (void*) -1) && (((uintptr_t) (p)) >= 4194304 && ((uintptr_t) (p)) < 0x800000000000ul))
#define ok_heap_addr(m) (m) < sbrk(0)

struct chunk_info_table 
{
  void *cur_chunk;
  void *prev_chunk;
  struct insert *ins;
  struct arena_bitmap_info *bitmap_info;
};

void inspect_uniqtype(struct uniqtype* out_type) {
  if (out_type){
    debug_printf("\n");
    debug_printf("kind: %d \n", UNIQTYPE_KIND(out_type));
    debug_printf("size in bytes? : %u \n", UNIQTYPE_SIZE_IN_BYTES(out_type));
    debug_printf("is ptr? %d \n", UNIQTYPE_IS_POINTER_TYPE(out_type));
    debug_printf("array length: %u \n", UNIQTYPE_ARRAY_LENGTH(out_type));
    debug_printf("array elemnt type: %s \n", UNIQTYPE_NAME(UNIQTYPE_ARRAY_ELEMENT_TYPE(out_type)));
    debug_printf("name: %s \n", UNIQTYPE_NAME(out_type));
    debug_printf("pointee type: %d \n", UNIQTYPE_POINTEE_TYPE(out_type));
    debug_printf("is composite type? %d \n", UNIQTYPE_IS_COMPOSITE_TYPE(out_type));
    debug_printf("\n");
  }
}

/*
*/
static void *view_chunk_uniqtype_metadata(struct chunk_info_table *tab, void **xarg3) 
{
    struct uniqtype *out_type;
    const void *site;
    liballocs_err_t err = extract_and_output_alloc_site_and_type(tab->ins, &out_type, &site);
    // if (err) printf("Extraction error! \n");
    if (!err){
      inspect_uniqtype(out_type);
    }
    // return NULL;
}

/*
*/
static void *find_allocated_chunk_for_userptr(struct chunk_info_table *tab, void **userptr) 
{
    void *cur_chunk = tab->cur_chunk;
    void *prev_chunk = tab->prev_chunk;
    if (tab->cur_chunk <= *userptr) return NULL; /* Continue scanning chunks */
    else {
      debug_printf("Target alloca chunk at %p and p_obj at %p\n", tab->prev_chunk, *userptr);
      return tab->prev_chunk; /* Stop scanning and return target chunk */
    }
}

/* 
* Iterates over all allocated chunks (and their insert fields) within heap memory 
* fn_arg: function ptr which can be given allocated chunk inserts as input
*/
void *scan_all_allocated_chunks(struct arena_bitmap_info* info, 
    void* (*chunk_fn)(struct chunk_info_table *, void**), void **fn_arg) {
  // pretend we're currently on 'one before the initial search start pos'
  unsigned long cur_bit_idx = -1;
  void *prev_usrchunk, *cur_userchunk = NULL; 
  void *ret = NULL; /* init chunk_fn return value */
  /* Iterate forward over bits */
  while ((unsigned long)-1 != (cur_bit_idx = bitmap_find_first_set1_geq_l(
                  info->bitmap, info->bitmap + info->nwords,
                  cur_bit_idx + 1, NULL)))
  {
          prev_usrchunk = cur_userchunk;
          cur_userchunk = (void*)((uintptr_t) info->bitmap_base_addr
                  + (cur_bit_idx * MALLOC_ALIGN));
          // bitmap_get_l(info->bitmap, ((uintptr_t) cur_userchunk - (uintptr_t) info->bitmap_base_addr) / MALLOC_ALIGN);

          // if (!ok_address(_gm_,cur_userchunk)) {
          //   // printf("Chunk: %p smaller than least address %p \n", cur_userchunk, _gm_.least_addr);
          //   continue;
          // }
          debug_printf("Walking a chunk at %p (bitmap array base addr: %p) idx %d\n", cur_userchunk,
                  (void*) info->bitmap_base_addr, (int) cur_bit_idx % 64);
          assert(IS_PLAUSIBLE_POINTER(cur_userchunk));
          // if (!IS_PLAUSIBLE_POINTER(cur_userchunk)) {
          //   debug_printf("Chunk: %p not plausible!\n", cur_userchunk);
          //   goto fail;
          // }
          struct insert *cur_insert = insert_for_chunk(cur_userchunk);
          assert((uintptr_t) cur_insert > (uintptr_t) cur_userchunk);
          assert(IS_PLAUSIBLE_POINTER(cur_insert));
          // if (!IS_PLAUSIBLE_POINTER(cur_insert)) {
          //   debug_printf("Insert: %p not plausible!\n", cur_insert);
          //   goto fail;
          // }
          debug_printf("flag: %u, site: 0x%lx and bits: %u \n", cur_insert->alloc_site_flag,cur_insert->alloc_site, cur_insert->un.bits);
          
          struct chunk_info_table tab = {
            .cur_chunk = cur_userchunk,
            .prev_chunk = prev_usrchunk,
            .ins = cur_insert,
            .bitmap_info = info
          };

          // view_chunk_uniqtype_metadata(&tab, fn_arg);

          if (chunk_fn) {
            ret = chunk_fn(&tab, fn_arg);
            if (ret) return ret;
          }
  }
  fail:
    return NULL;
}

void sanity_check_bitmap(struct arena_bitmap_info *info)
{
  unsigned long cur_bit_idx = -1;
  void *cur_userchunk;
  /* Iterate forward over bits */
  while ((unsigned long)-1 != (cur_bit_idx = bitmap_find_first_set1_geq_l(
                  info->bitmap, info->bitmap + info->nwords,
                  cur_bit_idx + 1, NULL)))
  {
          cur_userchunk = (void*)((uintptr_t) info->bitmap_base_addr
                  + (cur_bit_idx * MALLOC_ALIGN));
          assert((uintptr_t) insert_for_chunk(cur_userchunk) > (uintptr_t) cur_userchunk);
  }
}

/* For sanity checking
*/
void inspect_allocs() {
  printf("\nInspecting allocs --- \n");
  for (int i = 0; i < NBIGALLOCS; ++i) {
    // printf("Big alloc entry %i starts at %p ends at %p\n", i, big_allocations[i].begin, big_allocations[i].end);
    if (big_allocations[i].suballocator == &__generic_malloc_allocator)
    {
      printf("Big alloc entry %i starts at %p ends at %p\n", i, big_allocations[i].begin, big_allocations[i].end);
      struct big_allocation *arena = &big_allocations[i];
      struct arena_bitmap_info *info = arena->suballocator_private;
      bitmap_word_t *bitmap = info->bitmap;

      printf("Bitmap: %"PRIxPTR"\n", bitmap);
      for (int i= 0; i < info->nwords; ++i) {
        if (info->bitmap[i]){
          printf("Word at position %i is %lx \n", i, info->bitmap[i]);
        }
      }
      printf("\n");
      void *cur_userchunk;unsigned long cur_bit_idx = -1;
      /* Iterate forward over bits */
      while ((unsigned long)-1 != (cur_bit_idx = bitmap_find_first_set1_geq_l(
                      info->bitmap, info->bitmap + info->nwords,
                      cur_bit_idx + 1, NULL)))
      {
          cur_userchunk = (void*)((uintptr_t) info->bitmap_base_addr
                      + (cur_bit_idx * MALLOC_ALIGN));

          printf("Walking a chunk at %p (bitmap array base addr: %p) idx %d\n", cur_userchunk,
                  (void*) info->bitmap_base_addr, (int) cur_bit_idx % 64);
          struct insert *cur_insert = insert_for_chunk(cur_userchunk);
          printf("flag: %u, site: 0x%lx and bits: %u \n", cur_insert->alloc_site_flag,cur_insert->alloc_site, cur_insert->un.bits);
          
          struct uniqtype *out_type;
          const void *site;
          liballocs_err_t err = extract_and_output_alloc_site_and_type(cur_insert, &out_type, &site);
          if (!err && out_type) {
            printf("\n");
            printf("kind: %d \n", UNIQTYPE_KIND(out_type));
            printf("size in bytes? : %u \n", UNIQTYPE_SIZE_IN_BYTES(out_type));
            printf("is ptr? %d \n", UNIQTYPE_IS_POINTER_TYPE(out_type));
            printf("array length: %u \n", UNIQTYPE_ARRAY_LENGTH(out_type));
            printf("array elemnt type: %s \n", UNIQTYPE_NAME(UNIQTYPE_ARRAY_ELEMENT_TYPE(out_type)));
            printf("name: %s \n", UNIQTYPE_NAME(out_type));
            printf("pointee type: %d \n", UNIQTYPE_POINTEE_TYPE(out_type));
            printf("is composite type? %d \n", UNIQTYPE_IS_COMPOSITE_TYPE(out_type));
            printf("\n");
          }  
      }
      // sanity_check_bitmap(info);
    }
  }
  debug_printf("Finished inspection --- \n");
}


/* ----------------- uniqtype-DFS driver, callbacks, track_ptr fns & more ----------------*/

typedef int track_ptr_fn(void**, struct uniqtype**, void*);

struct parent_uniqtype__node
{
	void *parent_start;
	struct uniqtype *parent_t;
	unsigned long start_off;
	struct uniqtype *t_at_start;
	track_ptr_fn *ptr_cb;
	void *x_arg;
};
extern struct uniqtype *pointer_to___uniqtype__void;
extern struct uniqtype *pointer_to___uniqtype____uninterpreted_byte;

int __uniqtype_follow_ptr_heap(void **p_obj, struct uniqtype **p_t, void* xarg)
{
    /* 
    * Check if pointed to object (*p_obj) is in heap-allocated storage and set mark bit 
    * if true
    */
    struct big_allocation *arena = __lookup_bigalloc_from_root_by_suballocator(*p_obj,
      &__generic_malloc_allocator, NULL);
      if (arena){ /* If found, find insert for p_obj/chunk returned to applciation by dlmalloc */
        assert(ok_address(_gm_,*p_obj));
        // if (!ok_address(_gm_,*p_obj)) {
        //   printf("Chunk: %p smaller than least address %p. We are porbably the global sbrk snapshot.\n", *p_obj, _gm_.least_addr);
        //   return;
        // }
        debug_printf("Pageindex of p_obj: %d \n", pageindex[PAGENUM(*p_obj)]);
        debug_printf("sbrk(0): %p\n", sbrk(0));

        struct insert *cur_insert = insert_for_chunk(*p_obj);
        // assert(IS_PLAUSIBLE_POINTER(cur_insert));
        assert(ok_address(_gm_,cur_insert));
        assert(cur_insert < sbrk(0));
        // if (!IS_PLAUSIBLE_POINTER(cur_insert) || !ok_address(_gm_,cur_insert) || cur_insert > sbrk(0)) return;
        // if (!IS_PLAUSIBLE_POINTER(cur_insert) || cur_insert > sbrk(0)) return;
        debug_printf("flag: %u, site: %lx and bits: %u \n", cur_insert->alloc_site_flag,cur_insert->alloc_site,cur_insert->un.bits);
        
        /* Use bits field inside insert as marker */
        unsigned bits = cur_insert->un.bits;
        if (bits & 0x1 == 1) {debug_printf("Mark bit already set, possibly by interior pointer to chunk. Exiting follow_ptr_fn\n"); goto success;} // Check mark bit isn't set. That would be strange
        cur_insert->un.bits = bits | 0x1; // Flip LSB if 0
        // printf("New bits: %u \n",cur_insert->un.bits);

        /* continue DFS -
        * check inside chunks for pointers */
        uniqtype_dfs(*p_t, *p_obj, 0, __uniqtype_follow_ptr_heap);
        
        success:
          return 1; /* Success marking */
      }
      return 0; /* Pointer does not point to cached heap arena */
}

/*
* Inspired by
* https://github.com/stephenrkell/liballocs/blob/master/src/uniqtype-bfs.c
*/
static void explore_child_node(struct uniqtype *child_t, long child_offset_from_parent,
	struct parent_uniqtype__node *p_info)
{
  void *parent_start = p_info->parent_start;
	struct uniqtype *parent_t = p_info->parent_t;
	unsigned long start_off = p_info->start_off;
	struct uniqtype *t_at_start = p_info->t_at_start;
	track_ptr_fn *ptr_cb_for_marking = p_info->ptr_cb;
	void *x_arg = p_info->x_arg;

	/* If this subobject is a pointer, first check if it points to a valid object*/
	if (UNIQTYPE_IS_POINTER_TYPE(child_t))
	{
		struct uniqtype *grandchild_t = UNIQTYPE_POINTEE_TYPE(child_t);
		// get the address of the pointed-to object
		void *grandchild_start = *(void**)((uintptr_t) parent_start + start_off + child_offset_from_parent);
    /* We ignore null pointers, max addresses and insane pointers as defined by IS_PLAUSIBLE_POINTER*/    
		if (grandchild_start && IS_PLAUSIBLE_POINTER(grandchild_start))
		{
      debug_printf("\t%s_at_%p -> %s_at_%p;\n",
					NAME_FOR_UNIQTYPE(parent_t), parent_start,
					NAME_FOR_UNIQTYPE(grandchild_t), grandchild_start);
          
      ptr_cb_for_marking(&grandchild_start, &grandchild_t, x_arg);
		}
	}
  /* Recurse through composite or array type*/
	else if (UNIQTYPE_IS_COMPOSITE_TYPE(child_t))
	{
		DFS_explore_uniqtype_and__related(
        parent_start, parent_t, start_off + child_offset_from_parent,
        child_t, ptr_cb_for_marking,
        x_arg);
	}
}

/*
* Inspired by
* https://github.com/stephenrkell/liballocs/blob/master/src/uniqtype-bfs.c
*/
void DFS_explore_uniqtype_and__related(
	void *parent_start, struct uniqtype *parent_t, unsigned long start_off,
	struct uniqtype *t_at_start, track_ptr_fn *ptr_cb,
	void *xarg)
{
  /*Edge cases*/
	if (t_at_start == pointer_to___uniqtype__void) return;
  if (!t_at_start) return;

	assert(!t_at_start->make_precise);
	
  /* DFS finished here for non-composite and non-array types */
	if (!UNIQTYPE_HAS_SUBOBJECTS(t_at_start)) return;

  /* Similar trick as chunk_info_table 
  * We store parent node information in this table for use with every child node
  */
	struct parent_uniqtype__node p_info = {
		.parent_start = parent_start,
		.parent_t = parent_t,
		.start_off = start_off,
		.t_at_start = t_at_start,
		.ptr_cb = ptr_cb,
		.x_arg = xarg
	};

#define __uniqtype_thing(iter, type, offset) explore_child_node(type, offset, &p_info)
	UNIQTYPE_FOR_EACH_SUBOBJECT(t_at_start, __uniqtype_thing);
}

void uniqtype_dfs(struct uniqtype* t, void *obj_start, unsigned start_offset, track_ptr_fn *track_fn){
  DFS_explore_uniqtype_and__related( 
			obj_start, t, 
			start_offset, t,
			track_fn, NULL
		);
}

/* ------------------------ Stack unwinding and more  ----------------------- */

static int mark_from_frame(void *ip, void *sp, void *bp, track_ptr_fn *track_fn)
{
  debug_printf ("ip = %p, bp = %p, sp = %p\n", ip, bp, sp);

  // Get type information for this frame
  struct frame_uniqtype_and_offset s = pc_to_frame_uniqtype(ip);
  struct uniqtype *frame_desc = s.u;
		// The frame ALLOCATION base is different from the frame base. We
    // calculate its value using s.o
	unsigned char *frame_allocation_base = (unsigned char *) bp - s.o; 
  if (frame_desc){
    debug_printf("frame kind: %d \n", UNIQTYPE_KIND(frame_desc));
    uniqtype_dfs(frame_desc, frame_allocation_base, 0, track_fn);
    debug_printf("\n");
  }

	return 0; // keep going
}

static int liballocs_unwind_stack(int (*callback)(void *, void *, void *, void *), track_ptr_fn *xarg)
{
  debug_printf("Walking the stack ... \n");
	unw_cursor_t cursor; unw_context_t unw_context;
  /*
  * ip: instruction pointer
  * bp: base pointer
  * sp: stack pointer
  * nextframe_sp : the immediately higher frame's sp 
  */
	unw_word_t nextframe_sp = 0, sp, bp = 0, ip = 0;
	int unw_ret; /* return status of libunwind API calls */
	int ret = 0; /* callback(cb) return value */
  int have_bp = 0; /* bool set to 1 when %rbp succesfully read */
	
	/* Get an initial snapshot. */
	unw_ret = unw_getcontext(&unw_context);
  /* Initialize an unwind cursor based on this snapshot. */
	unw_init_local(&cursor, &unw_context);

#define INNERMOST_STACK_ADDRESS ((uintptr_t) MAXIMUM_USER_ADDRESS)
	while(nextframe_sp != INNERMOST_STACK_ADDRESS) // innermost frame
	{
		/* Get ip, bp and sp of current stack frame. */
		unw_get_reg(&cursor, UNW_REG_IP, &ip);
		unw_get_reg(&cursor, UNW_REG_SP, &sp);
		/* Try to get the bp, but may fail (due to compiler optimization) */
		unw_ret = unw_get_reg(&cursor, UNW_TDEP_BP, &bp); int have_bp = (unw_ret == 0);

    /* [DEBUGGING] Get symname of current procedure */
		char sym[256]; unw_word_t offset;
		debug_printf("0x%lx:", ip);
		if (unw_get_proc_name(&cursor, sym, sizeof(sym), &offset) == 0) {
			debug_printf(" (%s+0x%lx)\n", sym, offset);
		} else {
			debug_printf(" -- error: unable to obtain symbol name for this frame\n");
		}

    /* Move unwind cursor toward higher addresses */
		int unw_step_ret = unw_step(&cursor);
		if (unw_step_ret > 0)
		{
      /* If we don't have bp, just use nextframe_sp */
			unw_get_reg(&cursor, UNW_REG_SP, &nextframe_sp); 
      bp = have_bp ? bp : nextframe_sp;

      ret = callback((void*) ip, (void*) sp, (void*) bp, xarg);
		  if (ret) return ret;
		}
		else if (unw_step_ret == 0) /* Final frame reached */
		{
			nextframe_sp = INNERMOST_STACK_ADDRESS;
		}
		else /* Unwinding failure */
		{
			ret = -1;
			break;
		}
	}
	return ret;
}

/* ---------------------------- Global roots handlers  ----------------------- */

static void* metavector_follow_ptr_to_heap(void *obj_start, struct uniqtype* obj_type, void** xarg){
    const char* kind = obj_type->un.info.kind;
    const char* name = UNIQTYPE_NAME(obj_type);
      if ( kind != 0 && kind != 0x2 && kind != 0xa && kind != 0xc ) { 
        /* If not void, base, subprogram or subrange type, then continue. See DWARF debug format information 
        at https://developer.ibm.com/articles/au-dwarf-debug-format/ */
        
        if (kind == 0x8) { /* is an address */
          void *possible_ptr_to_heap = *(void**)(obj_start);
          if (!possible_ptr_to_heap) return NULL; /* Weird error - wrongly identified uniqtype? */
          debug_printf("Possible ptr to malloc: %p \n", possible_ptr_to_heap);

          struct uniqtype *pointed_to_static_t = UNIQTYPE_POINTEE_TYPE(obj_type);
          int ret = __uniqtype_follow_ptr_heap(obj_start, &pointed_to_static_t, NULL);
          return ret;
        }

        else { /* If we have a thing with a structure, recurse through it to find pointers */
          char ms[] = "malloc_state"; char mp[] = "malloc_params"; 
          int malloc_state_or_param;
          malloc_state_or_param = strcmp(name, ms) & strcmp(name,mp);
          if (!malloc_state_or_param) return NULL;

          uniqtype_dfs(obj_type, obj_start, 0, __uniqtype_follow_ptr_heap);
          debug_printf("\n");
        }
      }
        // struct insert *cur_insert = insert_for_chunk(x);
        // printf("flag: %u, site: %lx and bits: %u \n", cur_insert->alloc_site_flag,cur_insert->alloc_site,cur_insert->un.bits);
  return 0; /* Unreachable */
}

/*
*/
void scan_segments_of_executable(void* (*metavector_fn)(void*, struct uniqtype *, void**), void** xarg)
{
    debug_printf("Walking the link map of DSOs ... \n");
    struct link_map *l = _r_debug.r_map;
    // for (struct link_map *l = _r_debug.r_map; l; l = l->l_next)
    // {
      /* l_addr isn't guaranteed to be mapped by liballocs, so use l->l_ld */
      void *query_addr = l->l_ld;
      debug_printf("New DSO %s at l_ld: %p and l_addr:%p \n", l->l_name, query_addr, l->l_addr);
      // if (query_addr > (void *) 0x7f0000000000) continue; // Temporary hack
      struct big_allocation *dyn_mapping =__lookup_bigalloc_top_level(query_addr);
      struct big_allocation *static_file = __lookup_bigalloc_under(
        query_addr, &__static_file_allocator, dyn_mapping, NULL);
      assert(static_file);
      
      struct allocs_file_metadata *staticfile_allocmeta = static_file->allocator_private;
      for (struct big_allocation *bigseg = static_file->first_child; bigseg; bigseg = bigseg->next_sib) {
        assert(bigseg->allocated_by == &__static_segment_allocator);
        debug_printf("Segment starts at %p ends at %p\n", bigseg->begin, bigseg->end);

        /* Get segment metadata  */
        struct segment_metadata *seg_m = bigseg->allocator_private;
        
        /* Print out segment header information */
        ElfW(Phdr) *phdr = &staticfile_allocmeta->m.phdrs[seg_m->phdr_idx];
        debug_printf("p_type = %d, p_flags = %d\n", phdr->p_type, phdr->p_flags);

        /* If permissions not rw, wx, rwx then continue*/
        if (phdr->p_flags != 0x6 && phdr->p_flags != 0x3 && phdr->p_flags != 0x7) continue;

        for (struct big_allocation *bigsec = bigseg->first_child; bigsec; bigsec = bigsec->next_sib)
        {
          assert(bigsec->allocated_by == &__static_section_allocator);
          ElfW(Shdr) *shdr = (ElfW(Shdr) *)bigsec->allocator_private;

          debug_printf("Section starts at %p ends at %p\n", bigsec->begin, bigsec->end);

          debug_printf("Shrd metadata: flag: %d, type: %d, sh_addr: %d, sh_offset: %d, sh_size: %d\n",
                      shdr->sh_flags,shdr->sh_type,shdr->sh_addr,shdr->sh_offset,shdr->sh_size);
        
          // pass section header test
          if (shdr->sh_type != 8) continue;

          // Get metavector for this segment
          union sym_or_reloc_rec *metavector = seg_m->metavector;
          size_t metavector_size = seg_m->metavector_size;

        // we print the whole metavector
          for (unsigned i = 0; i < metavector_size / sizeof *metavector; ++i)
          {
            struct uniqtype *t = NULL;
            uintptr_t base_vaddr = vaddr_from_rec(&metavector[i], staticfile_allocmeta);
            void *x = (staticfile_allocmeta->m.l->l_addr + base_vaddr);
            // debug_printf("New metavector base at %p", base_vaddr);

            if (!metavector[i].is_reloc) {
              t = (struct uniqtype *)(((uintptr_t) metavector[i].sym.uniqtype_ptr_bits_no_lowbits)<<3);
            }
            // debug_printf("At %016lx there is a static alloc of kind %u, idx %08u, type %s\n",
						// 	x,
						// 	(unsigned) (metavector[i].is_reloc ? REC_RELOC : metavector[i].sym.kind),
						// 	(unsigned) (metavector[i].is_reloc ? 0 : metavector[i].sym.idx),
						// 	UNIQTYPE_NAME(
						// 		metavector[i].is_reloc ? NULL : 
            //     t
						// 	)
						// );
            
            // struct uniqtype *xtype = alloc_get_type(x);
            // struct allocator *xalloc = alloc_get_allocator(x);
            // unsigned long xsz = alloc_get_size(x);
            // debug_printf("At %p is a %s-allocated object of size %u, type %s, composite type? %lx and kind %d\n\n",
            //         x,
            //         xalloc ? xalloc->name : "no alloc found",
            //         xsz,
            //         xtype ? UNIQTYPE_NAME(xtype) : "no type found",
            //         xtype ? UNIQTYPE_IS_COMPOSITE_TYPE(xtype) : 0U,
            //         xtype ? UNIQTYPE_KIND(xtype) : 0
            //       );
            if (metavector_fn && t) {
              void* ret = NULL;
              ret = metavector_fn(x, t, xarg); /* What to do with ret? */
            }
          }
        }
      }
      debug_printf("\n");
    // }
}

/* ------------------------------- Mark-and-Sweep ----------------------------- */

struct big_allocation *malloc_arena = NULL;
void *WRITABLE_SECT_start, *WRITABLE_SECT_end = NULL;
struct big_allocation *loadable_segments_;
  /* 
  * Mark phase depth first search - 
  * iterate over root objects that contain pointers to heap storage which has been allocated to the user by malloc
  * and mark the chunks pointed to as 'reached'. Follow the child pointers
  * inside these chunks and mark them as 'reached' as well. At the end of the mark phase,
  * every marked object in the heap is black and every unmarked object is white.
  */ 
void mark_from_stack_and_register_roots() {
  debug_printf("\n");
  debug_printf("Marking allocs from stack frames and registers ... \n");
  int ret = liballocs_unwind_stack(mark_from_frame, __uniqtype_follow_ptr_heap);
  debug_printf("\n");
}

void mark_from_static_allocs() {
  debug_printf("\n");
  debug_printf("Marking static allocs ... \n");
  scan_segments_of_executable(metavector_follow_ptr_to_heap, NULL);
}

/* 
* Removes mark bit from a black object and frees white objects
*/
void *sweep_garbage(struct chunk_info_table *tab, void *xarg){
  // printf("Insert in sweep(): flag: %u, site: 0x%lx and bits: %u \n", tab->ins->alloc_site_flag,tab->ins->alloc_site, tab->ins->un.bits);
  // debug_printf("Flag is  %d\n", tab->ins->alloc_site_flag);
  if (tab->ins->un.bits & 0x1) /* If marked, unmark*/ {tab->ins->un.bits ^= 1;}
  else if (!tab->ins->alloc_site_flag) { return NULL; }
  //   debug_printf("Flag is %d\n", tab->ins->alloc_site_flag);
  //   return NULL; /* Ignore chunks that come from unclassified allocation sites */
  // }
  else { /* Free the unreachable garbage */
    struct big_allocation *arena = malloc_arena;
    debug_printf("*** Deleting entry for chunk %p, aligned end at %p\n\n", 
		tab->cur_chunk, (void*)((uintptr_t) tab->cur_chunk + MALLOC_ALIGN));
    // if (tab->prev_chunk && (void*)((uintptr_t) tab->prev_chunk + MALLOC_ALIGN > tab->cur_chunk)) {
    //   debug_printf("BINGO\n");
    // }
    __liballocs_bitmap_delete(arena, tab->cur_chunk);
    free(tab->cur_chunk);
  }
  return NULL; /* Continue collection */
}

void sweep() 
{
  debug_printf("\nSweeping allocs --- \n");
  if (!malloc_arena) { /* First collection run - we precompute arena for upcoming runs*/
    for (int i = 0; i < NBIGALLOCS; ++i) {
      // printf("Big alloc entry %i starts at %p ends at %p\n", i, big_allocations[i].begin, big_allocations[i].end);
      if (big_allocations[i].suballocator == &__generic_malloc_allocator 
            && big_allocations[i].allocated_by == &__brk_allocator)
      {
        debug_printf("Malloc arena big alloc entry %i starts at %p ends at %p\n", i, big_allocations[i].begin, big_allocations[i].end);
        
        malloc_arena = &big_allocations[i];
      }
    }
  }
    struct arena_bitmap_info *info = malloc_arena->suballocator_private;
    bitmap_word_t *bitmap = info->bitmap;

    debug_printf("Bitmap: %"PRIxPTR"\n", bitmap);
    for (int i= 0; i < info->nwords; ++i) {
      if (info->bitmap[i]){
        debug_printf("Word at position %i is %lx \n", i, info->bitmap[i]);
      }
    }
    debug_printf("nwords: %lu \n", info->nwords);
    scan_all_allocated_chunks(info,sweep_garbage,NULL);

  debug_printf("Finished sweep --- \n");

  for (int i= 0; i < info->nwords; ++i) {
    if (info->bitmap[i]){
      debug_printf("Word at position %i is %lx \n", i, info->bitmap[i]);
    }
  }

  // inspect_allocs();
}

#define mark_And_sweep() {mark_from_stack_and_register_roots();mark_from_static_allocs();sweep();}


/* ------------------------------- GC_Malloc ----------------------------- */

#ifndef NOGC

#ifndef HAVE_MORECORE
#ifdef COUNTER
int GC_counter = COUNTER; /* Every 5 mallocs, we garbage collect*/
#endif
#endif
void* GC_Malloc(size_t bytes) {

  debug_printf("You're inside GC_Malloc \n");
#ifndef HAVE_MORECORE 
#ifdef COUNTER
  GC_counter = GC_counter - 1;
  if (GC_counter == 0){
    GC_counter = -1;
    mark_And_sweep();
    GC_counter = COUNTER;
  }
#endif
#endif
  
  void *rptr, *brk, *rptr2 = NULL;
  rptr = malloc(bytes);
#ifdef HAVE_MORECORE
  if (!rptr) { /* brk threshold reached */
    mark_And_sweep(); // GC
    rptr2 = malloc(bytes); /* try again */

    if (!rptr2) { /* Bad - ran out of heap space */
      errno = ENOMEM;
      printf("Out of space; errno code: %d\n", errno); exit(0);
    }
  }
#endif
  liballocs_err_t err = extract_and_output_alloc_site_and_type(insert_for_chunk(rptr), NULL, NULL);
  if (!err) {
    debug_printf("flag: %u \n", insert_for_chunk(rptr)->alloc_site_flag);
  }
  return rptr;
}

#endif

#ifdef NOGC
void* GC_Malloc(size_t bytes) {
  return malloc(bytes);
}
#endif

void exp_collect() {mark_And_sweep();}



/* ---------------------- GC_Realloc, GC_Calloc and GC_Free ------------------------- */

void* GC_Calloc(size_t nmemb, size_t bytes) {
  // debug_printf("You're inside GC_Calloc\n");
  // void* rptr;
  // rptr = calloc(nmemb, bytes);
  // liballocs_err_t err = extract_and_output_alloc_site_and_type(insert_for_chunk(rptr), NULL, NULL);
  // if (!err) {
  //   debug_printf("flag: %u \n", insert_for_chunk(rptr)->alloc_site_flag);
  // }
  // return rptr;
  return calloc(nmemb, bytes);
}

void* GC_Realloc(void* addr, size_t bytes) {
  // debug_printf("You're inside GC_Realloc\n");
  // void* rptr;
  // rptr = realloc(addr, bytes);
  // liballocs_err_t err = extract_and_output_alloc_site_and_type(insert_for_chunk(rptr), NULL, NULL);
  // if (!err) {
  //   debug_printf("flag: %u \n", insert_for_chunk(rptr)->alloc_site_flag);
  // }
  // return rptr;
  return realloc(addr, bytes);
}

void GC_Free(void* addr){
  return;
}
