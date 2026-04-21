#include <stdio.h>
#include <stdint.h>

#include <stdio.h>
#include <string.h>
#include <stdbool.h>
#include "os_log.h"

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

static Block_t *g_pxFreeListHead = NULL;

void *pHeapFree(void *pvPointerName);
void *pHeapMalloc(size_t xWantedSize);


void heapPrintBlocks(void);
void heapPrintFreeList(void);
void printSeparator(const char *title);
