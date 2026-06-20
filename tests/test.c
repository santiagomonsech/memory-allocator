#include "../src/malloc.h"
#include <stdio.h>
#include <assert.h>



int main(void) {
    printf("=== ma_malloc tests ===\n");

    void *p = ma_malloc(64);
    assert(p != NULL && "basic allocation should succeed");
    printf("[PASS] ma_malloc(64)\n");

    ma_free(p);
    printf("[PASS] ma_free\n");

    printf("\nAll tests passed!\n");
    return 0;
}
