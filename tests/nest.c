#include <stdio.h>
#include <inttypes.h>
#include "GC_funcs.h"
// #include "boehm/gc.h"

extern int mallocs, collections;
/* -------------------------------- Test program ----------------------------- */

struct simplemaze {
  char* name;
};
struct maze {
  int num;
  double num2;
  char *name;
  struct simplemaze *inner;
};
struct maze *nested_mazes() {
  struct maze *l1;
  struct simplemaze *l2;
  l1 = malloc(sizeof(struct maze));
  l2 = malloc(sizeof(struct simplemaze));
  l2->name = "inner";
  l1->num = 5;
  l1->num2 = 16;
  l1->name = "outer";
  l1->inner = l2;
  printf("Live maze: l1: %p, l2: %p, l1->num: %p, l1->inner addr: %p, distance from l1 to l1->inner: %d\n",
        l1,l2,l1->num,&(l1->inner),(uintptr_t)(&l1->inner) - (uintptr_t)l1);
  return l1;
}

struct simplepark {
  char *name;
  int loc;
  struct simplemaze *smallmaze;
};
struct park {
  char *name;
  int loc;
  struct simplepark *simp;
};

void lose_malloc(){
  // Create and lose some mallocs
  void *l1 = malloc(sizeof(double));
  void *l2 = malloc(2 * sizeof(double));
  void *l3 = malloc(3 * sizeof(int));
  struct maze *lmaze = malloc(sizeof(struct maze));
  lmaze->name = "lose";
  lmaze->num = 5;
  lmaze->num2 = 16;
  printf("Losing pointers: l1: %p, l2: %p, l3: %p, lmaze: %p\n",
        l1,l2,l3,lmaze);
}

//globs
void *GLOBAL_VAR_malloc;
int glob;
int main(int argc, char **argv)
{
  // write_string("With call size: \n");
  // raw_syscall_write_string("Testing..\n");
  glob = 9 * 20;
  // GLOBAL_VAR_malloc = nested_mallocs();
  GLOBAL_VAR_malloc = malloc(sizeof(double));
  // lose_malloc();

  /** Mazes **/
  struct maze *mz = nested_mazes();
  struct maze *mz2 = malloc(sizeof(struct maze));
  mz2->name = "super outer maze";
  mz2->num = 3;
  mz2->inner = mz;
  printf("Maze check\n");
  printf("MAZE2 : %p, mz2->name: %s, mz2->num: %p, mz2->num2: %f, mz2->inner: %p \n",
        mz2, mz2->name, mz2->num, mz2->num2, mz2->inner);  
  
  /** Parks **/
  struct park *park1 = malloc(sizeof(struct park));
  struct simplepark *s_park = malloc(sizeof(struct simplepark));
  park1->name = "outer park";
  park1->loc = 29;
  park1->simp = s_park;
  s_park->name = "inner park";
  s_park->smallmaze = mz->inner;
  printf("Park check\n");
  printf("Park1 : %p, park1->name: %s, park1->loc: %p, park1->simplepark: %p\n",
        park1, park1->name, park1->loc, park1->simp);  
  printf("s_park : %p, s_park->name: %s, s_park->loc: %p, s_park->innermaze: %p\n",
        s_park, s_park->name, s_park->loc, s_park->smallmaze);

  /* Misc */
  void *p = malloc(5 * sizeof(int));
  void *chmalloc = malloc(50 * sizeof(char));
  lose_malloc();


  printf("Live pointers NOW: p: %p, chmalloc: %p, maze1: %p, maze2: %p, global_malloc: %p \n",
        p,chmalloc,mz,mz2,GLOBAL_VAR_malloc);
  
  for (int i = 0; i < 200000; ++i){
    void *p = malloc(sizeof(int));
    void *q = malloc(sizeof(double));
    void *m = malloc(sizeof(char));
  }
  exp_collect(); /* Call collector */
  
  printf("Live pointers AFTER: p: %p, chmalloc: %p, maze1: %p, maze2: %p, global_malloc: %p \n",
        p,chmalloc,mz,mz2,GLOBAL_VAR_malloc);
  printf("Maze check\n");
  printf("maze l1: l1->name: %s, l1->num: %p, l1->num2: %f, l1->inner: %p \n",
        mz->name, mz->num, mz->num2, mz->inner);
  printf("maze l2: l2->name: %s",
        mz->inner->name);
  printf("MAZE2 : mz2->name: %s, mz2->num: %p, mz2->num2: %f, mz2->inner: %p \n",
        mz2->name, mz2->num, mz2->num2, mz2->inner);
  printf("Total mallocs: %d Total collections: %d", mallocs, collections);
  // printf("maze l1: l1->name: %s, l1->num: %p, l1->num2: %f, l1->inner: %p \n",
  //       GLOBAL_VAR_malloc->name, GLOBAL_VAR_malloc->num, GLOBAL_VAR_malloc->num2, GLOBAL_VAR_malloc->inner);
  // printf("maze l2: l2->name: %s",
  //       GLOBAL_VAR_malloc->inner->name);
  inspect_allocs();
  printf("SUCCESS \n");

//   struct elf {};
//   typedef struct elf {} Elf64_Word;
//   typedef struct elf {} Elf64_Half;
//   typedef struct elf {} Elf64_Addr;
//   typedef struct elf {} Elf64_Xword;
//   typedef struct elf {} Elf64_Off;

//   typedef struct {
//         Elf64_Word      sh_name;
//         Elf64_Word      sh_type;
//         Elf64_Xword     sh_flags;
//         Elf64_Addr      sh_addr;
//         Elf64_Off       sh_offset;
//         Elf64_Xword     sh_size;
//         Elf64_Word      sh_link;
//         Elf64_Word      sh_info;
//         Elf64_Xword     sh_addralign;
//         Elf64_Xword     sh_entsize;
//    } Elf64_Shdr;


//    typedef struct {
//         Elf64_Word      p_type;
//         Elf64_Word      p_flags;
//         Elf64_Off       p_offset;
//         Elf64_Addr      p_vaddr;
//         Elf64_Addr      p_paddr;
//         Elf64_Xword     p_filesz;
//         Elf64_Xword     p_memsz;
//         Elf64_Xword     p_align;
//     } Elf64_Phdr;


  return 0;
}
