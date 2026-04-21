#include "heap4_allocator.h"

int main(void)
{
    printSeparator("INIT (trigger lazy init)");
    void *tmp = pHeapMalloc(1);
    pHeapFree(tmp);
    heapPrintBlocks();
    heapPrintFreeList();

    /* ============================= */
    /* TEST 1: BASIC SPLIT */
    /* ============================= */
    void *p1 = pHeapMalloc(100);
    void *p2 = pHeapMalloc(200);

    printSeparator("AFTER MALLOC p1(100), p2(200)");
    heapPrintBlocks();
    heapPrintFreeList();

    /* ============================= */
    /* TEST 2: FREE + NO MERGE */
    /* ============================= */
    pHeapFree(p1);

    printSeparator("FREE p1 (no merge)");
    heapPrintBlocks();
    heapPrintFreeList();

    /* ============================= */
    /* TEST 3: MERGE WITH NEXT */
    /* ============================= */
    pHeapFree(p2);

    printSeparator("FREE p2 (merge with p1)");
    heapPrintBlocks();
    heapPrintFreeList();

    /* ============================= */
    /* TEST 4: SPLIT AGAIN */
    /* ============================= */
    void *a = pHeapMalloc(100);
    void *b = pHeapMalloc(100);
    void *c = pHeapMalloc(100);

    printSeparator("ALLOC a b c");
    heapPrintBlocks();
    heapPrintFreeList();

    /* ============================= */
    /* TEST 5: FREE MIDDLE (NO MERGE) */
    /* ============================= */
    pHeapFree(b);

    printSeparator("FREE b (no merge)");
    heapPrintBlocks();
    heapPrintFreeList();

    /* ============================= */
    /* TEST 6: MERGE BOTH SIDES */
    /* ============================= */
    pHeapFree(a);
    pHeapFree(c);

    printSeparator("FREE a + c (merge all)");
    heapPrintBlocks();
    heapPrintFreeList();

    /* ============================= */
    /* TEST 7: REUSE FREED BLOCK */
    /* ============================= */
    void *d = pHeapMalloc(250);

    printSeparator("ALLOC d(250) reuse merged block");
    heapPrintBlocks();
    heapPrintFreeList();

    /* ============================= */
    /* TEST 8: FRAGMENTATION */
    /* ============================= */
    void *arr[10];

    for (int i = 0; i < 10; i++)
        arr[i] = pHeapMalloc(50);

    for (int i = 0; i < 10; i += 2)
        pHeapFree(arr[i]);

    printSeparator("FRAGMENTATION");
    heapPrintBlocks();
    heapPrintFreeList();

    /* ============================= */
    /* TEST 9: FULL MERGE AFTER STRESS */
    /* ============================= */
    for (int i = 1; i < 10; i += 2)
        pHeapFree(arr[i]);

    pHeapFree(d);

    printSeparator("FULL MERGE AFTER STRESS");
    heapPrintBlocks();
    heapPrintFreeList();

    /* ============================= */
    /* TEST 10: ALLOC SIZE = 0 */
    /* ============================= */
    void *z = pHeapMalloc(0);
    LOG_DBG("ALLOC size 0 -> %p (expect NULL)", z);

    /* ============================= */
    /* TEST 11: ALLOC TOO LARGE */
    /* ============================= */
    void *big = pHeapMalloc(configTOTAL_HEAP_SIZE * 2);
    LOG_DBG("ALLOC too large -> %p (expect NULL)", big);

    /* ============================= */
    /* TEST 12: DOUBLE FREE */
    /* ============================= */
    void *df = pHeapMalloc(100);
    pHeapFree(df);

    printSeparator("DOUBLE FREE TEST");
    pHeapFree(df);
    heapPrintBlocks();
    heapPrintFreeList();

    /* ============================= */
    /* TEST 13: INVALID POINTER FREE */
    /* ============================= */
    int x;
    printSeparator("INVALID FREE TEST");
    pHeapFree(&x);
    heapPrintBlocks();
    heapPrintFreeList();

    /* ============================= */
    /* TEST 14: RANDOM ALLOC/FREE */
    /* ============================= */
    printSeparator("RANDOM STRESS");

    void *randArr[20];

    for (int i = 0; i < 20; i++)
        randArr[i] = pHeapMalloc((i % 5 + 1) * 30);

    for (int i = 0; i < 20; i += 3)
        pHeapFree(randArr[i]);

    for (int i = 1; i < 20; i += 4)
        pHeapFree(randArr[i]);

    heapPrintBlocks();
    heapPrintFreeList();

    /* ============================= */
    /* TEST 15: FREE ALL */
    /* ============================= */
    for (int i = 0; i < 20; i++)
        pHeapFree(randArr[i]);

    printSeparator("FINAL FULL MERGE");
    heapPrintBlocks();
    heapPrintFreeList();

    printSeparator("DONE");
    return 0;
}