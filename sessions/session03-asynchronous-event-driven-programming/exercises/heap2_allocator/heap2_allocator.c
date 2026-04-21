#include <stdio.h>
#include <stdint.h>
#include <stdbool.h>
#include <string.h>

#define configTOTAL_HEAP_SIZE 1024 * 4
#define configADJUSTED_HEAP_SIZE (configTOTAL_HEAP_SIZE - portBYTE_ALIGNMENT)
#define portBYTE_ALIGNMENT_MASK (0x0003)
#define portBYTE_ALIGNMENT 4
#define heapSIZE_MAX (~((size_t)0))
#define heapADD_WILL_OVERFLOW(a, b) ((a) > (heapSIZE_MAX - (b)))
#define heapBITS_PER_BYTE ((size_t)8)
#define heapBLOCK_ALLOCATED_BITMASK (((size_t)1) << ((sizeof(size_t) * heapBITS_PER_BYTE) - 1))
#define heapBLOCK_SIZE_IS_VALID(xBlockSize) (((xBlockSize) & heapBLOCK_ALLOCATED_BITMASK) == 0)
#define heapMINIMUM_BLOCK_SIZE ((size_t)(xHeapStructSize * 2))
#define heapALLOCATE_BLOCK(pxBlock) ((pxBlock->xBlockSize) |= heapBLOCK_ALLOCATED_BITMASK)
#define heapBLOCK_IS_ALLOCATED(pxBlock) (((pxBlock->xBlockSize) & heapBLOCK_ALLOCATED_BITMASK) != 0)
#define heapFREE_BLOCK(pxBlock) ((pxBlock->xBlockSize) &= ~heapBLOCK_ALLOCATED_BITMASK)

static size_t xFreeBytesRemaining = configADJUSTED_HEAP_SIZE;
static uint8_t ucHeap[configTOTAL_HEAP_SIZE];

// Define a block structure with a pointer to the next free block
typedef struct Block
{
    struct Block *pxNextFreeBlock;
    size_t xBlockSize;
} Block_t;

static const size_t xHeapStructSize = ((sizeof(Block_t) + (size_t)(portBYTE_ALIGNMENT - 1)) & ~((size_t)portBYTE_ALIGNMENT_MASK));
static bool xHeapHasBeenInitialised = false;

static Block_t *pxHead = NULL;

#define prvInsertBlockIntoFreeList(pxBlockToInsert)                  \
    {                                                                \
        Block_t *pxPrev;                                             \
        Block_t *pxCurr;                                             \
                                                                     \
        if (pxHead == NULL)                                          \
        {                                                            \
            pxHead = pxBlockToInsert;                                \
            pxBlockToInsert->pxNextFreeBlock = NULL;                 \
        }                                                            \
        else if (pxBlockToInsert->xBlockSize <= pxHead->xBlockSize)  \
        {                                                            \
            pxBlockToInsert->pxNextFreeBlock = pxHead;               \
            pxHead = pxBlockToInsert;                                \
        }                                                            \
        else                                                         \
        {                                                            \
            pxPrev = pxHead;                                         \
            pxCurr = pxHead->pxNextFreeBlock;                        \
                                                                     \
            while (pxCurr != NULL &&                                 \
                   pxCurr->xBlockSize < pxBlockToInsert->xBlockSize) \
            {                                                        \
                pxPrev = pxCurr;                                     \
                pxCurr = pxCurr->pxNextFreeBlock;                    \
            }                                                        \
                                                                     \
            pxBlockToInsert->pxNextFreeBlock = pxCurr;               \
            pxPrev->pxNextFreeBlock = pxBlockToInsert;               \
        }                                                            \
    }

static void heapInit(void)
{
    uint8_t *pucAlignedHeap;

    pucAlignedHeap = (uint8_t *)((size_t)(ucHeap + portBYTE_ALIGNMENT - 1) & (~((size_t)portBYTE_ALIGNMENT_MASK)));

    pxHead = (Block_t *)pucAlignedHeap;
    pxHead->xBlockSize = configADJUSTED_HEAP_SIZE;
    pxHead->pxNextFreeBlock = NULL;
};

void *pvMalloc(size_t xWantedSize)
{
    Block_t *pxBlock;
    Block_t *pxPreviousBlock = NULL;
    Block_t *pxNewBlockLink;
    size_t xAdditionalRequiredSize;
    void *pvReturn = NULL;

    if (xWantedSize > 0)
    {
        if (heapADD_WILL_OVERFLOW(xWantedSize, xHeapStructSize) == 0)
        {
            xWantedSize += xHeapStructSize;

            if ((xWantedSize & portBYTE_ALIGNMENT_MASK) != 0x00)
            {
                xAdditionalRequiredSize = portBYTE_ALIGNMENT - (xWantedSize & portBYTE_ALIGNMENT_MASK);

                if (heapADD_WILL_OVERFLOW(xWantedSize, xAdditionalRequiredSize) == 0)
                {
                    xWantedSize += xAdditionalRequiredSize;
                }
                else
                {
                    xWantedSize = 0;
                }
            }
            else
            {
            }
        }
        else
        {
            xWantedSize = 0;
        }
    }
    else
    {
    }

    if (xHeapHasBeenInitialised == false)
    {
        heapInit();
        xHeapHasBeenInitialised = true;
    }

    if (heapBLOCK_SIZE_IS_VALID(xWantedSize) != 0)
    {
        if ((xWantedSize > 0) && (xWantedSize <= xFreeBytesRemaining))
        {
            pxBlock = pxHead;

            while (pxBlock != NULL)
            {
                if (pxBlock->xBlockSize >= xWantedSize)
                {
                    break;
                }

                pxPreviousBlock = pxBlock;
                pxBlock = pxBlock->pxNextFreeBlock;
            }

            if (pxBlock != NULL)
            {
                if (pxPreviousBlock == NULL)
                {
                    pxHead = pxBlock->pxNextFreeBlock;
                }
                else
                {
                    pxPreviousBlock->pxNextFreeBlock = pxBlock->pxNextFreeBlock;
                }
                pvReturn = (void *)(((uint8_t *)pxBlock) + xHeapStructSize);

                if ((pxBlock->xBlockSize - xWantedSize) > heapMINIMUM_BLOCK_SIZE)
                {
                    pxNewBlockLink = (void *)(((uint8_t *)pxBlock) + xWantedSize);
                    pxNewBlockLink->xBlockSize = pxBlock->xBlockSize - xWantedSize;
                    pxBlock->xBlockSize = xWantedSize;

                    prvInsertBlockIntoFreeList((pxNewBlockLink));
                }
                xFreeBytesRemaining -= pxBlock->xBlockSize;

                heapALLOCATE_BLOCK(pxBlock);
                pxBlock->pxNextFreeBlock = NULL;
            }
        }
    }

    return pvReturn; // Data
}

void vFree(void *pBlock)
{
    uint8_t *puc = (uint8_t *)pBlock;
    Block_t *pLink;

    if (pBlock != NULL)
    {
        puc -= xHeapStructSize;

        pLink = (void *)puc;

        if (heapBLOCK_IS_ALLOCATED(pLink) != 0)
        {
            if (pLink->pxNextFreeBlock == NULL)
            {
                heapFREE_BLOCK(pLink);
                (void)memset(puc + xHeapStructSize, 0, pLink->xBlockSize - xHeapStructSize);
                prvInsertBlockIntoFreeList(((Block_t *)pLink));
                xFreeBytesRemaining += pLink->xBlockSize;
            }
        }
    }
}

void heapPrintBlocks()
{
    uint8_t *ptr = ucHeap;
    uint8_t *end = ucHeap + configADJUSTED_HEAP_SIZE;

    printf("\n===== HEAP BLOCK LIST =====\n");

    while (ptr < end)
    {
        Block_t *blk = (Block_t *)ptr;

        size_t raw = blk->xBlockSize;
        size_t size = raw & ~heapBLOCK_ALLOCATED_BITMASK;
        int isAlloc = (raw & heapBLOCK_ALLOCATED_BITMASK) != 0;

        if (size == 0 || size > configTOTAL_HEAP_SIZE)
        {
            printf("[ERROR] Corrupted at %p | raw=%zu\n", blk, raw);
            break;
        }

        printf("Block: addr %p | size %-4zu | state %s\n",
               blk,
               size,
               isAlloc ? "BUSY" : "FREE");

        ptr += size;
    }

    printf("===========================\n");
}

void heapVisualize()
{
    const int BAR_SZ = 20;

    uint8_t *ptr = ucHeap;
    uint8_t *end = ucHeap + configADJUSTED_HEAP_SIZE;

    printf("\n========= HEAP VISUAL =========\n");

    while (ptr < end)
    {
        Block_t *blk = (Block_t *)ptr;

        size_t raw = blk->xBlockSize;
        size_t size = raw & ~heapBLOCK_ALLOCATED_BITMASK;
        int isAlloc = (raw & heapBLOCK_ALLOCATED_BITMASK) != 0;

        if (size == 0 || size > configTOTAL_HEAP_SIZE)
        {
            printf("[ERROR] Corrupted at %p | raw=%zu\n", blk, raw);
            break;
        }

        printf("%p +", blk);
        for (int i = 0; i < BAR_SZ; i++)
            printf("-");
        printf("+\n");

        printf("           | %c | size=%-4zu |\n",
               isAlloc ? 'A' : 'F',
               size);

        int lines = size / 128;
        if (lines > 5)
            lines = 5;

        for (int i = 0; i < lines; i++)
        {
            printf("           |");
            for (int j = 0; j < BAR_SZ; j++)
                printf(" ");
            printf("|\n");
        }

        ptr += size;
    }

    printf("%p +", end);
    for (int i = 0; i < BAR_SZ; i++)
        printf("-");
    printf("+\n");

    printf("================================\n");
}

void heapPrintFreeList()
{
    printf("\n------ FREE LIST ------\n");

    Block_t *curr = pxHead;

    while (curr != NULL)
    {
        printf("[%p | %zu] -> ", curr,
               curr->xBlockSize & ~heapBLOCK_ALLOCATED_BITMASK);
        curr = curr->pxNextFreeBlock;
    }

    printf("NULL\n");
}
void printAll()
{
    heapPrintBlocks();
    // heapVisualize();
    heapPrintFreeList();
}

int main()
{
    heapInit();

    void *p1 = pvMalloc(100);
    void *p2 = pvMalloc(200);
    void *p3 = pvMalloc(300);

    printf("\n=== AFTER MALLOC ===\n");
    printAll();

    vFree(p2);
    printf("\n=== AFTER FREE p2 ===\n");
    printAll();

    vFree(p1);
    printf("\n=== AFTER FREE p1 ===\n");
    printAll();

    vFree(p3);
    printf("\n=== AFTER FREE p3 ===\n");
    printAll();

    return 0;
}