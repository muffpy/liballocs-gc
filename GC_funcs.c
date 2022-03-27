#include <allocs.h>
#include <stdio.h>
#include "dlmalloc.h"
#include "GC_funcs.h"

void* GC_malloc(size_t bytes) {
  printf("You're inside GC_malloc \n");
  return dlmalloc(bytes);
}

void* GC_calloc(size_t nmemb, size_t bytes) {
  return dlcalloc(nmemb, bytes);
}

void* GC_realloc(void* addr, size_t bytes) {
  return dlrealloc(addr, bytes);
}

void GC_free(void* addr){
  return dlfree(addr);
}