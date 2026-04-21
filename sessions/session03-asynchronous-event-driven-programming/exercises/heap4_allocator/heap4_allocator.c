#include "heap4_allocator.h"

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
                    LOG_DBG("Final size after alignment: %u", xWantedSize);
                }
                else
                {
                    xWantedSize = 0U;
                    LOG_WARN("Memory allocation failed: overflow occurred during alignment padding");
                }
            }
            else
            {
                LOG_DBG("No alignment required. Size after alignment: %u", xWantedSize);
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
                        LOG_DBG("No split performed. Remaining size too small, allocaterd entire block: %u", pxCurrentBlock->xBlockSize);
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

    uint8_t *pu8CurrentBlock = g_u8Heap;
    uint8_t *pu8End = g_u8Heap + configADJUSTED_HEAP_SIZE;

    LOG_DBG("[HEAP BLOCKS]");

    while (pu8CurrentBlock < pu8End)
    {
        Block_t *pxBlock = (Block_t *)pu8CurrentBlock;

        size_t xBlockSize = pxBlock->xBlockSize;
        size_t xSize = xBlockSize & ~BLOCK_ALLOCATED_BITMASK;

        if (xSize == 0 || xSize > configADJUSTED_HEAP_SIZE)
        {
            LOG_WARN("Corrupted at %p", pxBlock);
            break;
        }

        LOG_DBG("addr=%p | size=%-4zu | %s",pxBlock, xSize,  BLOCK_IS_ALLOCATED(pxBlock) ? "ALLOC" : "FREE");

        pu8CurrentBlock += xSize;
    }
}

void heapPrintFreeList(void)
{
    extern Block_t *g_pxFreeListHead;

    LOG_DBG("[FREE LIST]");

    Block_t *pxCurrentBlock = g_pxFreeListHead;

    while (pxCurrentBlock != NULL)
    {
        LOG_DBG("%p (%zu) -> ", pxCurrentBlock, FREE_BLOCK(pxCurrentBlock));

        pxCurrentBlock = pxCurrentBlock->pNextFreeBlock;
    }

    LOG_DBG("NULL");
}

void printSeparator(const char *title)
{
    LOG_DBG("================ %s ================", title);
}

