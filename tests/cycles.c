/* Cyclic garbage benchmark test designed by Rahul Ramaka.

   A major drawback of naive garbage colelction methods like
   reference counting is the inability to free cyclic structures
   in the program since their reference counts never reach
   zero. Mark&Sweep defines liveness in the form of reachability
   via a chain of pointers from the root set. The following
   allocates a cyclic linked-list structure within a function 
   and does not return a reference back to the callee 
   in effect "losing" this structure to the heap.

   This benchmark was created for liballocs-gc as a way to 
   demonstrate that capabilities of mark & sweep collection.
   Using the printed out pointer information to stdout, we 
   check which pointed-to chunks were reclaimed and which 
   were not.
*/

#include <stdio.h>
#include "../GC_funcs.h"
// #include "boehm/gc.h"
// #define malloc(n) GC_malloc(n)

struct Link {
    struct Link* front;
    struct Link* back;
    int idx;
};

int loseCycle() {
    // Allocate links for a circular chain
    struct Link* l1 = malloc(sizeof(struct Link)); l1->idx = 0;
    struct Link* l2 = malloc(sizeof(struct Link)); l2->idx = 1;
    struct Link* l3 = malloc(sizeof(struct Link)); l3->idx = 2;
    struct Link* l4 = malloc(sizeof(struct Link)); l4->idx = 3;
    struct Link* l5 = malloc(sizeof(struct Link)); l5->idx = 4;
    struct Link* l6 = malloc(sizeof(struct Link)); l6->idx = 5;

    // Create doubly-linked circular chain
    l1->front = l2; l2->front = l3; l3->front = l4; l4->front = l5; l5->front = l6; l6->front = l1;
    l1->back = l6; l2->back = l1; l3->back = l2; l4->back = l3; l5->back = l4; l6->back = l5;
    

    // printf("Cycle:\n");
    // printf("%p <-> %p <-> %p <-> %p <-> %p <-> %p\n",l1,l2,l3,l4,l5,l6);
    // printf("\n");

    return 0; /* Successfully lost Links */
}

int main() {

    // GC_INIT();

    int ret;
    for (int i = 0; i < 100000; ++i) /* Lose a cycle and collect every time */
        ret = loseCycle();
    
    exp_collect();
    // GC_gcollect(); /*Boehm gc*/


    // inspect_allocs();

    return 0;
}