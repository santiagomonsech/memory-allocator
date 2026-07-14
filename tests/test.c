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

    printf("TESTING FIRST FIT ALLOCATION\n");
    void *p1 = ma_malloc(32);
    assert(p1 != NULL && "first fit allocation should succeed");
    printf("[PASS] ma_malloc(32)\n");

    void *p2 = ma_malloc(16);
    assert(p2 != NULL && "first fit allocation should succeed");
    printf("[PASS] ma_malloc(16)\n");

    ma_free(p1);
    printf("[PASS] ma_free\n");

    printf("\nAll tests passed!\n");
    return 0;
}
