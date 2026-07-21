#include "../src/malloc.h"
#include <stdio.h>
#include <assert.h>

static void test_coalesce_next(void) {
    printf("\n=== test_coalesce_next (merge with next block) ===\n");

    void *wall = ma_malloc(4096);
    assert(wall != NULL);

    void *a = ma_malloc(200);
    assert(a != NULL);
    void *b = ma_malloc(150);
    assert(b != NULL);

    ma_free(b);
    ma_free(a);

    void *c = ma_malloc(300);
    assert(c != NULL);
    assert(c == a && "merge-with-next should reuse address a");
    printf("[PASS] merge-with-next: c reuses the merged block in  a\n");

    ma_free(c);
}

static void test_coalesce_both(void) {
    printf("\n=== test_coalesce_both (merge with both neighbors) ===\n");

    void *wall = ma_malloc(4096);
    assert(wall != NULL);

    void *x = ma_malloc(500);
    assert(x != NULL);
    void *y = ma_malloc(400);
    assert(y != NULL);
    void *z = ma_malloc(450);
    assert(z != NULL);

    ma_free(x);
    ma_free(z);
    ma_free(y);

    void *w = ma_malloc(1000);
    assert(w != NULL);
    assert(w == x && "merge-both should reuse address x");
    printf("[PASS] merge-both: w reuses the merged block in x\n");

    ma_free(w);
}

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
    assert (p1 == p);
    printf("[PASS] ma_malloc(32) is reusing coalesced block\n");

    void *p2 = ma_malloc(16);
    assert(p2 != NULL && "first fit allocation should succeed");
    printf("[PASS] ma_malloc(16)\n");

    void *p3 = ma_malloc(16);
    assert(p3 != NULL && "first fit allocation should succeed");
    printf("[PASS] ma_malloc(16)\n");

    ma_free(p1);
    printf("[PASS] ma_free\n");
    ma_free(p2);
    printf("[PASS] ma_free\n");
    ma_free(p3);
    printf("[PASS] ma_free\n");

    void *p4 = ma_malloc(96);
    assert(p4 != NULL && "first fit allocation should succeed");
    assert(p4 == p1 && "first fit allocation should reuse coalesced block");
    printf("[PASS] ma_malloc(96) is reusing coalesced block\n");

    ma_free(p4);
    printf("[PASS] ma_free\n");

    test_coalesce_next();
    test_coalesce_both();

    printf("\nAll tests passed!\n");
    return 0;
}
