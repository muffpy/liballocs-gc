/* Compiled with plain gcc */

#include <stdio.h>
#include <stdint.h>


int main(){
    // printf("hey \n");
    // printf("%zu\n",sizeof (uintptr_t));
    // printf("%zu\n",(sizeof (void*))<<3);
    for (int i = 0; i < 1000; ++i){
        void *p = malloc(sizeof(int));
    }
    return 0;
}