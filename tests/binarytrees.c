/* The Computer Language Benchmarks Game
 * https://salsa.debian.org/benchmarksgame-team/benchmarksgame/
 *
 * Contributed by Eckehard Berns
 * Based on code by Kevin Carson
 * *reset*
 */

#include <stdlib.h>
#include <stdio.h>
#include "../GC_funcs.h"
// #include "boehm/gc.h"
// #define malloc(n) GC_malloc(n)

typedef struct node {
   struct node *left, *right;
} node;

static node *
new_node(node *left, node *right)
{
   node *ret;

   ret = malloc(sizeof(node));
   ret->left = left;
   ret->right = right;

   return ret;
}

static long
item_check(node *tree)
{
   if (tree->left == NULL)
      return 1;
   else
      return 1 + item_check(tree->left) +
          item_check(tree->right);
}

static node *
bottom_up_tree(int depth)
{
   if (depth > 0)
      return new_node(bottom_up_tree(depth - 1),
          bottom_up_tree(depth - 1));
   else
      return new_node(NULL, NULL);
}

/* Removed delete_tree */

struct worker_args {
   long iter, check;
   int depth;
   pthread_t id;
   struct worker_args *next;
};

static void *
check_tree_of_depth(void *_args)
{
   struct worker_args *args = _args;
   long i, iter, check, depth;
   node *tmp;

   iter = args->iter;
   depth = args->depth;

   check = 0;
   for (i = 1; i <= iter; i++) {
      tmp = bottom_up_tree(depth);
      check += item_check(tmp);
   }

   args->check = check;
   return NULL;
}

int
main(int ac, char **av)
{
   // GC_INIT(); /* Bohem gc init */

   struct worker_args *args;
   int n, depth, mindepth, maxdepth;

   n = ac > 1 ? atoi(av[1]) : 10;
   if (n < 1) {
      fprintf(stderr, "Wrong argument.\n");
      exit(1);
   }

   mindepth = 4;
   maxdepth = mindepth + 2 > n ? mindepth + 2 : n;

   for (depth = mindepth; depth <= maxdepth; depth += 2) {

      args = malloc(sizeof(struct worker_args));
      args->iter = 1 << (maxdepth - depth + mindepth);
      args->depth = depth;
      args->next = NULL;

      check_tree_of_depth(args);
      printf("%ld\t trees of depth %d\t check: %ld\n",
          args->iter, args->depth, args->check);
      /* Not in the original benchmark game code */
      exp_collect();
      // timed_collect();
      // GC_gcollect();
      printf("Finished collection for depth: %d\n", args->depth);
   }

   /* Not in the original benchmark game code */
   inspect_allocs();
   printf("Success!");

   return 0;
}