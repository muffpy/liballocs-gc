#include <stdio.h>
#include "GC_funcs.h"
// #include "boehm/gc.h"

/* -------------------------------- Test program ----------------------------- */

void lose_malloc(){
  // Create and lose some mallocs
  void *l1 = malloc(sizeof(double));
  void *l2 = malloc(2 * sizeof(double));
  void *l3 = malloc(3 * sizeof(int));
  void *l4 = malloc(4 * sizeof(int));
  void *l5 = malloc(5 * sizeof(int));
  printf("Losing pointers: l1: %p, l2: %p, l3: %p, l4: %p, l5: %p\n",
        l1,l2,l3,l4,l5);
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
  l1->num2 = 14;
  l1->inner = l2;
  l2->name = "inner";
  return l1;
}
//globs
void *GLOBAL_VAR_malloc;
int glob;
int main(int argc, char **argv)
{
  // write_string("With call size: \n");
  // raw_syscall_write_string("Testing..\n");
  glob = 9 * 20;
  GLOBAL_VAR_malloc = malloc(sizeof(double));
  // lose_malloc();
  struct maze *l3 = nested_mallocs();
  // another_one = l1;
  void *p = malloc(5 * sizeof(int));
  void *chmalloc = malloc(50 * sizeof(char));
  
  free(p);

  lose_malloc();

  for (int i = 0; i < 10000; ++i){
    void *p = malloc(sizeof(int));
    void *q = malloc(sizeof(double));
    void *m = malloc(sizeof(char));
  }
  exp_collect(); /* Call collector */
  
  printf("Live pointers: p: %p, chmalloc: %p, maze l3: %p, global_malloc: %p \n",
        p,chmalloc,l3,GLOBAL_VAR_malloc);
  inspect_allocs();
  printf("SUCCESS \n");
  return 0;
  
  // struct elf {};
  // typedef struct elf {} Elf64_Word;
  // typedef struct elf {} Elf64_Half;
  // typedef struct elf {} Elf64_Addr;
  // typedef struct elf {} Elf64_Xword;

  // typedef struct elf64_sym {
  //   Elf64_Word      st_name; /* Offset into the symbol string table which holds symbol names*/
  //   unsigned char   st_info: 8; /* Type (4 bits) and binding (4 bits) attributes */
  //   unsigned char   st_other; /* No definite meaning, 0 */
  //   Elf64_Half      st_shndx; /* Offset into the seciton header table */
  //   Elf64_Addr      st_value; /* Address or absolute value of associated symbol */
  //   Elf64_Xword     st_size; /* Associated object size (in bytes) */
  // } Elf64_Sym;

}
