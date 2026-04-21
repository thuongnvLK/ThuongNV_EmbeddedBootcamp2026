#include <stdio.h>
#include <stdint.h>

#include <stdio.h>
#include <string.h>
#include <stdbool.h>

/* Log level */
#define LOG_LEVEL_NONE 0
#define LOG_LEVEL_ERROR 1
#define LOG_LEVEL_WARN 2
#define LOG_LEVEL_INFO 3

#ifndef LOG_LEVEL
#define LOG_LEVEL LOG_LEVEL_NONE
#endif

/* Get short file name */
#define LOG_FILE_NAME (__builtin_strrchr(__FILE__, '/') ? __builtin_strrchr(__FILE__, '/') + 1 : __FILE__)

/* Base macro */
#define LOG_BASE(level_str, fmt, ...)    \
    printf("[%s] %s:%d:%s(): " fmt "\n", \
           level_str, LOG_FILE_NAME, __LINE__, __func__, ##__VA_ARGS__)

/* Log macros */
#if LOG_LEVEL >= LOG_LEVEL_ERROR
#define LOG_ERROR(fmt, ...) LOG_BASE("ERROR", fmt, ##__VA_ARGS__)
#else
#define LOG_ERROR(...) ((void)0)
#endif

#if LOG_LEVEL >= LOG_LEVEL_WARN
#define LOG_WARN(fmt, ...) LOG_BASE("WARN ", fmt, ##__VA_ARGS__)
#else
#define LOG_WARN(...) ((void)0)
#endif

#if LOG_LEVEL >= LOG_LEVEL_INFO
#define LOG_INFO(fmt, ...) LOG_BASE("INFO ", fmt, ##__VA_ARGS__)
#else
#define LOG_INFO(...) ((void)0)
#endif

#define HEAP_ALIGNMENT_BYTES 4U
#define HEAP_ALIGNMENT_MASK (HEAP_ALIGNMENT_BYTES - 1U)
#define BITS_PER_BYTE ((size_t)8)
#define BLOCK_ALLOCATED_BITMASK (((size_t)1) << ((sizeof(size_t) * BITS_PER_BYTE) - 1))
#define BLOCK_SIZE_IS_VALID(xBlockSize) (((xBlockSize) & BLOCK_ALLOCATED_BITMASK) == 0)
#define ADD_WILL_OVERFLOW(a, b) ((a) > (SIZE_MAX - (b)))
#define BLOCK_IS_ALLOCATED(pxBlock) (((pxBlock->xBlockSize) & BLOCK_ALLOCATED_BITMASK) != 0)
#define ALLOCATE_BLOCK(pxBlock) ((pxBlock->xBlockSize) |= BLOCK_ALLOCATED_BITMASK)
#define FREE_BLOCK(pxBlock) ((pxBlock->xBlockSize) &= ~(BLOCK_ALLOCATED_BITMASK))
#define configTOTAL_HEAP_SIZE 4096
#define configADJUSTED_HEAP_SIZE (configTOTAL_HEAP_SIZE - HEAP_ALIGNMENT_BYTES)

typedef struct Block_t
{
    struct Block_t *pNextFreeBlock;
    size_t xBlockSize;
} Block_t;

static uint8_t g_u8Heap[configTOTAL_HEAP_SIZE];

// Round up the size of Block_t (heap block header) to the nearest multiple of the alignment.
static const size_t g_xHeapStructSize = (sizeof(Block_t) + ((size_t)(HEAP_ALIGNMENT_BYTES - 1))) & ~((size_t)(HEAP_ALIGNMENT_MASK));

#define MINIMUM_BLOCK_SIZE ((size_t)(g_xHeapStructSize << 1))

static size_t g_xFreeBytesRemaining = (size_t)0U;
static size_t g_xMinimumEverFreeBytesRemaining = (size_t)0U;
static size_t g_xNumberOfSuccessfulAllocations = (size_t)0U;
static size_t g_xNumberOfSuccessfulFrees = (size_t)0U;
static bool g_xHeapHasBeenInitialised = false;

static Block_t g_xStart;
static Block_t *g_pxEnd = NULL;
static Block_t *g_pxFreeListHead = NULL;

static void HeapInit(void)
{

    uint8_t *pu8AlignedHead = NULL;

    /*
    * MEMORY ALIGNMENT VISUALIZATION
    * 
    * Target: Align g_u8Heap to the next boundary (e.g., 8-byte alignment)
    *
    * Address:   0x00    0x01    0x02    0x03    0x04    0x05    0x06    0x07    0x08    0x09    0x0A
    *            |-------|-------|-------|-------|-------|-------|-------|-------|-------|-------|
    *            ^               ^                                       ^
    *            |               |                                       |
    *      Aligned Bound    g_u8Heap (Start)                        pu8AlignedHead (Result)
    *      (e.g. 0x00)      (e.g. 0x02)                             (e.g. 0x08)
    *
    *                            [------- Padding (Ignored Bytes) -------][===== USEABLE HEAP =====...
    * 
    */
    pu8AlignedHead = (uint8_t *)((size_t)(g_u8Heap + HEAP_ALIGNMENT_BYTES - 1) & ~((size_t)(HEAP_ALIGNMENT_MASK)));


    /*
    * g_pxFreeListHead
    * |
    * V
    * +----------+------------------------------------------+
    * |  HEADER  |                 BODY                     | ----> NULL
    * +----------+------------------------------------------+
    * (Metadata)        (Total Free Memory Space)          (End of List)
    */
    g_pxFreeListHead = (Block_t *)pu8AlignedHead;
    g_pxFreeListHead->xBlockSize = configADJUSTED_HEAP_SIZE;
    g_pxFreeListHead->pNextFreeBlock = NULL;

    g_xMinimumEverFreeBytesRemaining = g_pxFreeListHead->xBlockSize;
    g_xFreeBytesRemaining = g_pxFreeListHead->xBlockSize;
}

static void HeapInsertBlockIntoFreeList(Block_t *pxBlockToInsert)
{
    Block_t *pxCurrentBlock = g_pxFreeListHead;

    if (g_pxFreeListHead == NULL)
    {
        g_pxFreeListHead = pxBlockToInsert;
        pxBlockToInsert->pNextFreeBlock = NULL;
    }
    else if (pxBlockToInsert < g_pxFreeListHead)
    {
        if ((uint8_t *)pxBlockToInsert + pxBlockToInsert->xBlockSize == (uint8_t *)pxCurrentBlock)
        {
            pxBlockToInsert->xBlockSize += pxCurrentBlock->xBlockSize;
            pxBlockToInsert->pNextFreeBlock = pxCurrentBlock->pNextFreeBlock;
        }
        else
        {
            pxBlockToInsert->pNextFreeBlock = pxCurrentBlock;
        }

        g_pxFreeListHead = pxBlockToInsert;
    }
    else
    {

        while (pxCurrentBlock->pNextFreeBlock)
        {
            if (pxCurrentBlock->pNextFreeBlock > pxBlockToInsert)
            {
                break;
            }

            pxCurrentBlock = pxCurrentBlock->pNextFreeBlock;
        }

        if ((((uint8_t *)(pxCurrentBlock) + pxCurrentBlock->xBlockSize)) == (uint8_t *)pxBlockToInsert)
        {
            pxCurrentBlock->xBlockSize += pxBlockToInsert->xBlockSize;
            pxBlockToInsert = pxCurrentBlock;
        }
        else
        {
        }

        if (pxCurrentBlock->pNextFreeBlock != NULL &&
            (uint8_t *)pxBlockToInsert + pxBlockToInsert->xBlockSize == (uint8_t *)pxCurrentBlock->pNextFreeBlock)
        {
            if (pxCurrentBlock->pNextFreeBlock != NULL)
            {
                pxBlockToInsert->xBlockSize += pxCurrentBlock->pNextFreeBlock->xBlockSize;
                pxBlockToInsert->pNextFreeBlock = pxCurrentBlock->pNextFreeBlock->pNextFreeBlock;
            }
            else
            {
                pxBlockToInsert->pNextFreeBlock = NULL;
            }
        }
        else
        {
            pxBlockToInsert->pNextFreeBlock = pxCurrentBlock->pNextFreeBlock;
        }

        if (pxCurrentBlock != pxBlockToInsert)
        {
            pxCurrentBlock->pNextFreeBlock = pxBlockToInsert;
        }
        else
        {
        }
    }
}

void *pHeapMalloc(size_t xWantedSize)
{
    Block_t *pxCurrentBlock = NULL;
    Block_t *pxPreviousBlock = NULL;
    Block_t *pxNewBlock = NULL;
    void *pvReturn = NULL;
    size_t xAdditionalRequiredSize = 0U;

    if (xWantedSize > 0)
    {
        if (ADD_WILL_OVERFLOW(xWantedSize, g_xHeapStructSize) == 0)
        {
            xWantedSize += g_xHeapStructSize;

            if ((xWantedSize & HEAP_ALIGNMENT_MASK) != 0x00)
            {
                xAdditionalRequiredSize = (HEAP_ALIGNMENT_BYTES) - (xWantedSize & HEAP_ALIGNMENT_MASK);

                if (ADD_WILL_OVERFLOW(xWantedSize, xAdditionalRequiredSize) == 0)
                {
                    xWantedSize += xAdditionalRequiredSize;
                    LOG_INFO("Final size after alignment: %u", xWantedSize);
                }
                else
                {
                    xWantedSize = 0U;
                    LOG_ERROR("Memory allocation failed: overflow occurred during alignment padding");
                }
            }
            else
            {
                LOG_INFO("No alignment required. Size after alignment: %u", xWantedSize);
            }
        }
        else
        {
            xWantedSize = 0U;
        }
    }
    else
    {
        LOG_ERROR("Memory allocation failed: overflow occurred when adding heap struct size");
    }

    {
        if (g_xHeapHasBeenInitialised == false)
        {
            HeapInit();
            g_xHeapHasBeenInitialised = true;
        }

        if (BLOCK_SIZE_IS_VALID(xWantedSize) != 0)
        {
            if ((xWantedSize > 0) && (xWantedSize <= g_xFreeBytesRemaining))
            {
                pxCurrentBlock = g_pxFreeListHead;

                while ((pxCurrentBlock->xBlockSize < xWantedSize) && (pxCurrentBlock->pNextFreeBlock != NULL))
                {
                    pxPreviousBlock = pxCurrentBlock;
                    pxCurrentBlock = pxCurrentBlock->pNextFreeBlock;
                }

                if (pxCurrentBlock != NULL)
                {

                    if ((pxCurrentBlock->xBlockSize - xWantedSize) > MINIMUM_BLOCK_SIZE)
                    {
                        pxNewBlock = (void *)((uint8_t *)(pxCurrentBlock) + xWantedSize);
                        pxNewBlock->xBlockSize = pxCurrentBlock->xBlockSize - xWantedSize;
                        pxCurrentBlock->xBlockSize = xWantedSize;

                        pxNewBlock->pNextFreeBlock = pxCurrentBlock->pNextFreeBlock;
                        pxCurrentBlock->pNextFreeBlock = pxNewBlock;
                        // HeapInsertBlockIntoFreeList(pxNewBlock);
                    }
                    else
                    {
                        LOG_INFO("No split performed. Remaining size too small, allocaterd entire block: %u", pxCurrentBlock->xBlockSize);
                    }

                    if (pxPreviousBlock == NULL)
                    {
                        g_pxFreeListHead = pxCurrentBlock->pNextFreeBlock;
                    }
                    else
                    {
                        pxPreviousBlock->pNextFreeBlock = pxCurrentBlock->pNextFreeBlock;
                    }

                    pvReturn = (void *)((uint8_t *)(pxCurrentBlock) + g_xHeapStructSize);

                    g_xFreeBytesRemaining -= pxCurrentBlock->xBlockSize;

                    if (g_xFreeBytesRemaining < g_xMinimumEverFreeBytesRemaining)
                    {
                        g_xMinimumEverFreeBytesRemaining = g_xFreeBytesRemaining;
                    }
                    else
                    {
                    }

                    ALLOCATE_BLOCK(pxCurrentBlock);
                    pxCurrentBlock->pNextFreeBlock = NULL;
                    g_xNumberOfSuccessfulAllocations++;
                }
                else
                {
                }
            }
            else
            {
            }
        }
        else
        {
        }
    }
    return pvReturn;
}

void *pHeapFree(void *pvPointerName)
{
    uint8_t *pu8Pointer = (uint8_t *)pvPointerName;
    Block_t *pxCurrentBlock = NULL;

    if (pvPointerName != NULL)
    {
        pu8Pointer -= g_xHeapStructSize;
        pxCurrentBlock = (void *)pu8Pointer;

        if (BLOCK_IS_ALLOCATED(pxCurrentBlock) != 0)
        {
            if (pxCurrentBlock->pNextFreeBlock == NULL)
            {
                FREE_BLOCK(pxCurrentBlock);
                (void)memset(pu8Pointer + g_xHeapStructSize, 0, pxCurrentBlock->xBlockSize - g_xHeapStructSize);
            }

            {
                g_xFreeBytesRemaining += pxCurrentBlock->xBlockSize;
                HeapInsertBlockIntoFreeList((Block_t *)pxCurrentBlock);
                g_xNumberOfSuccessfulFrees++;
            }
        }
    }
}

void heapPrintBlocks(void)
{

    uint8_t *ptr = g_u8Heap;
    uint8_t *end = g_u8Heap + configADJUSTED_HEAP_SIZE;

    printf("\n[HEAP BLOCKS]\n");

    while (ptr < end)
    {
        Block_t *blk = (Block_t *)ptr;

        size_t raw = blk->xBlockSize;
        size_t size = raw & ~BLOCK_ALLOCATED_BITMASK;
        int isAlloc = (raw & BLOCK_ALLOCATED_BITMASK) != 0;

        if (size == 0 || size > configADJUSTED_HEAP_SIZE)
        {
            printf("[ERROR] Corrupted at %p\n", blk);
            break;
        }

        printf("addr=%p | size=%-4zu | %s\n",
               blk,
               size,
               isAlloc ? "ALLOC" : "FREE");

        ptr += size;
    }
}

void heapPrintFreeList(void)
{
    extern Block_t *g_pxFreeListHead;

    printf("\n[FREE LIST]\n");

    Block_t *curr = g_pxFreeListHead;

    while (curr != NULL)
    {
        printf("%p (%zu) -> ",
               curr,
               curr->xBlockSize & ~BLOCK_ALLOCATED_BITMASK);

        curr = curr->pNextFreeBlock;
    }

    printf("NULL\n");
}

static void printSeparator(const char *title)
{
    printf("\n================ %s ================\n", title);
}
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
    printf("\nALLOC size 0 -> %p (expect NULL)\n", z);

    /* ============================= */
    /* TEST 11: ALLOC TOO LARGE */
    /* ============================= */
    void *big = pHeapMalloc(configTOTAL_HEAP_SIZE * 2);
    printf("\nALLOC too large -> %p (expect NULL)\n", big);

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