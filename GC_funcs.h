void* GC_Malloc(size_t);
void* GC_Calloc(size_t, size_t);
void* GC_Realloc(void*, size_t);
void GC_Free(void*);
void inspect_allocs();
void exp_collect();
void timed_collect();