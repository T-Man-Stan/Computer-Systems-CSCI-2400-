#include <stdio.h>
#include <stdlib.h>
#include <assert.h>
#include <unistd.h>
#include "memlib.h"
#include "mm.h"
#include <memory.h>

team_t team = {
  /* Team name */
  "Treviathan",
  /* First member's full name */
  "Trevor Stanley",
  /* First member's email address */
  "trst9490@colorado.edu",
  /* Second member's full name (leave blank if none) */
  "none",
  /* Second member's email address (leave blank if none) */
  "none"
};


/* Macros for pointer arithmetic to keep other code cleaner.  Casting
   to a char* has the effect that pointer arithmetic happens at the
   byte granularity (i.e. POINTER_ADD(0x1, 1) would be 0x2).  (By
   default, incrementing a pointer in C has the effect of incrementing
   it by the size of the type to which it points (e.g. Blk_Info).) */
#define POINTER_ADD(p,x) ((char*)(p) + (x)) 
#define POINTER_SUB(p,x) ((char*)(p) - (x))


/******** FREE LIST IMPLEMENTATION ***********************************/


/* A Blk_Info contains information about a block, including the size
   and usage tags, as well as pointers to the next and previous blocks
   in the free list.  Explanation and some code courtesy of GitHub. 
   Note that the next and prev pointers and the boundary tag are only
   needed when the block is free.  To achieve better utilization, mm_malloc
   should use the space for next and prev as part of the space it returns.
   
   +--------------+
   | sizeAndTags  |  <-  Blk_Info ptrs in free list pnt here
   |  (header)    |
   +--------------+
   |     next     |  <-  Ptrs returned by mm_malloc pnt here
   +--------------+
   |     prev     |
   +--------------+
   |  space and   |
   |   padding    |
   |     ...      |
   |     ...      |
   +--------------+
   | boundary tag |
   |  (footer)    |
   +--------------+
*/
struct Blk_Info {
  // Size of the block (in the high bits) and tags for whether the
  // block and its predecessor in memory are in use.  See the SIZE()
  // and TAG macros, below, for more details.
  uint32_t sizeAndTags;
  // Pointer to the next block in the free list.
  struct Blk_Info* next;
  // Pointer to the previous block in the free list.
  struct Blk_Info* prev;
};
typedef struct Blk_Info Blk_Info;


/* Pointer to the first Blk_Info in the free list, the list's head. 
   
   A pointer to the head of the free list in this implementation is
   always stored in the first word in the heap.  mem_heap_lo() returns
   a pointer to the first word in the heap, so we cast the result of
   mem_heap_lo() to a Blk_Info** (a pointer to a pointer to
   Blk_Info) and dereference this to get a pointer to the first
   Blk_Info in the free list. */
#define FREE_LIST_HEAD *((Blk_Info **)mem_heap_lo())

/* Size of a word on this architecture. */
#define WORD_SIZE sizeof(void*)

/* Minimum block size (to account for size header, next ptr, prev ptr,
   and boundary tag) */
#define MIN_BLOCK_SIZE (sizeof(Blk_Info) + WORD_SIZE)

/* Alignment of blocks returned by mm_malloc. */
#define ALIGNMENT 8

/* SIZE(blockInfo->sizeAndTags) extracts the size of a 'sizeAndTags' field.
   Also, calling SIZE(size) selects just the higher bits of 'size' to ensure
   that 'size' is properly aligned.  We align 'size' so we can use the low
   bits of the sizeAndTags field to tag a block as free/used, etc, like this:
   
      sizeAndTags:
      +-------------------------------------------+
      | 63 | 62 | 61 | 60 |  . . . .  | 2 | 1 | 0 |
      +-------------------------------------------+
        ^                                       ^
      high bit                               low bit
   Since ALIGNMENT == 8, we reserve the low 3 bits of sizeAndTags for tag
   bits, and we use bits 3-63 to store the size.
   Bit 0 (2^0 == 1): TAG_USED
   Bit 1 (2^1 == 2): TAG_PRECEDING_USED
*/
#define SIZE(x) ((x) & ~(ALIGNMENT - 1))

/* TAG_USED is the bit mask used in sizeAndTags to mark a block as used. */
#define TAG_USED 1 

/* TAG_PRECEDING_USED is the bit mask used in sizeAndTags to indicate
   that the block preceding it in memory is used. (used in turn for
   coalescing).  If the previous block is not used, we can learn the size
   of the previous block from its boundary tag */
#define TAG_PRECEDING_USED 2


/* Find a free block of the requested size in the free list.  Returns
   NULL if no free block is large enough. */
static void * searchFreeList(uint32_t reqSize) {   
  Blk_Info* freeBlock;

  freeBlock = FREE_LIST_HEAD;
  while (freeBlock != NULL){
    if (SIZE(freeBlock->sizeAndTags) >= reqSize) {
      return freeBlock;
    } else {
      freeBlock = freeBlock->next;
    }
  }
  return NULL;
}
           
/* Insert freeBlock at the head of the list.  (LIFO) */
static void insertFreeBlock(Blk_Info* freeBlock) {
  Blk_Info* oldHead = FREE_LIST_HEAD;
  freeBlock->next = oldHead;
  if (oldHead != NULL) {
    oldHead->prev = freeBlock;
  }
  //  freeBlock->prev = NULL;
  FREE_LIST_HEAD = freeBlock;
}      

/* Remove a free block from the free list. */
static void removeFreeBlock(Blk_Info* freeBlock) {
  Blk_Info *nextFree, *prevFree;
  
  nextFree = freeBlock->next;
  prevFree = freeBlock->prev;

  // If the next block is not null, patch its prev pointer.
  if (nextFree != NULL) {
    nextFree->prev = prevFree;
  }

  // If we're removing the head of the free list, set the head to be
  // the next block, otherwise patch the previous block's next pointer.
  if (freeBlock == FREE_LIST_HEAD) {
    FREE_LIST_HEAD = nextFree;
  } else {
    prevFree->next = nextFree;
  }
}

/* Coalesce 'oldBlock' with any preceding or following free blocks. */
static void coalesceFreeBlock(Blk_Info* oldBlock) {
  Blk_Info *blockCursor;
  Blk_Info *newBlock;
  Blk_Info *freeBlock;
  // size of old block
  uint32_t oldSize = SIZE(oldBlock->sizeAndTags);
  // running sum to be size of final coalesced block
  uint32_t newSize = oldSize;

  // Coalesce with any preceding free block
  blockCursor = oldBlock;
  while ((blockCursor->sizeAndTags & TAG_PRECEDING_USED)==0) { 
    // While the block preceding this one in memory (not the
    // prev. block in the free list) is free:

    // Get the size of the previous block from its boundary tag.
    uint32_t size = SIZE(*((uint32_t*)POINTER_SUB(blockCursor, WORD_SIZE)));
    // Use this size to find the block info for that block.
    freeBlock = (Blk_Info*)POINTER_SUB(blockCursor, size);
    // Remove that block from free list.
    removeFreeBlock(freeBlock);

    // Count that block's size and update the current block pointer.
    newSize += size;
    blockCursor = freeBlock;
  }
  newBlock = blockCursor;

  // Coalesce with any following free block.
  // Start with the block following this one in memory
  blockCursor = (Blk_Info*)POINTER_ADD(oldBlock, oldSize);
  while ((blockCursor->sizeAndTags & TAG_USED)==0) {
    // While the block is free:

    uint32_t size = SIZE(blockCursor->sizeAndTags);
    // Remove it from the free list.
    removeFreeBlock(blockCursor);
    // Count its size and step to the following block.
    newSize += size;
    blockCursor = (Blk_Info*)POINTER_ADD(blockCursor, size);
  }
  
  // If the block actually grew, remove the old entry from the free
  // list and add the new entry.
  if (newSize != oldSize) {
    // Remove the original block from the free list
    removeFreeBlock(oldBlock);

    // Save the new size in the block info and in the boundary tag
    // and tag it to show the preceding block is used (otherwise, it
    // would have become part of this one!).
    newBlock->sizeAndTags = newSize | TAG_PRECEDING_USED;
    // The boundary tag of the preceding block is the word immediately
    // preceding block in memory where we left off advancing blockCursor.
    *(uint32_t*)POINTER_SUB(blockCursor, WORD_SIZE) = newSize | TAG_PRECEDING_USED;  

    // Put the new block in the free list.
    insertFreeBlock(newBlock);
  }
  return;
}

/* Get more heap space of size at least reqSize. */
static void requestMoreSpace(uint32_t reqSize) {
  uint32_t pagesize = mem_pagesize();
  uint32_t numPages = (reqSize + pagesize - 1) / pagesize;
  Blk_Info *newBlock;
  uint32_t totalSize = numPages * pagesize;
  uint32_t prevLastWordMask;

  void* mem_sbrk_result = mem_sbrk(totalSize);
  if ((uint32_t)mem_sbrk_result == -1) {
    printf("ERROR: mem_sbrk failed in requestMoreSpace\n");
    exit(0);
  }
  newBlock = (Blk_Info*)POINTER_SUB(mem_sbrk_result, WORD_SIZE);

  /* initialize header, inherit TAG_PRECEDING_USED status from the
     previously useless last word however, reset the fake TAG_USED
     bit */
  prevLastWordMask = newBlock->sizeAndTags & TAG_PRECEDING_USED;
  newBlock->sizeAndTags = totalSize | prevLastWordMask;
  // Initialize boundary tag.
  ((Blk_Info*)POINTER_ADD(newBlock, totalSize - WORD_SIZE))->sizeAndTags = 
    totalSize | prevLastWordMask;

  /* initialize "new" useless last word
     the previous block is free at this moment
     but this word is useless, so its use bit is set
     This trick lets us do the "normal" check even at the end of
     the heap and avoid a special check to see if the following
     block is the end of the heap... */
  *((uint32_t*)POINTER_ADD(newBlock, totalSize)) = TAG_USED;

  // Add the new block to the free list and immediately coalesce newly
  // allocated memory space
  insertFreeBlock(newBlock);
  coalesceFreeBlock(newBlock);
}

/* Print the heap by iterating through it as an implicit free list. */
static void examine_heap() {
  Blk_Info *block;

  /* print to stderr so output isn't buffered and not output if we crash */
  fprintf(stderr, "FREE_LIST_HEAD: %p\n", (void *)FREE_LIST_HEAD);

  for(block = (Blk_Info *)POINTER_ADD(mem_heap_lo(), WORD_SIZE); /* first block on heap */
      SIZE(block->sizeAndTags) != 0 && ((void*) block < mem_heap_hi());
      block = (Blk_Info *)POINTER_ADD(block, SIZE(block->sizeAndTags))) {

    /* print out common block attributes */
    fprintf(stderr, "%p: %ld %ld %ld\t",
    (void *)block,
    SIZE(block->sizeAndTags),
    block->sizeAndTags & TAG_PRECEDING_USED,
    block->sizeAndTags & TAG_USED);

    /* and allocated/free specific data */
    if (block->sizeAndTags & TAG_USED) {
      fprintf(stderr, "ALLOCATED\n");
    } else {
      fprintf(stderr, "FREE\tnext: %p, prev: %p\n",
      (void *)block->next,
      (void *)block->prev);
    }
  }
  fprintf(stderr, "END OF HEAP\n\n");
}

/* Initialize the allocator. */
int mm_init () {
  // Head of the free list.
  Blk_Info *firstFreeBlock;

  // Initial heap size: WORD_SIZE byte heap-header (stores pointer to head
  // of free list), MIN_BLOCK_SIZE bytes of space, WORD_SIZE byte heap-footer.
  uint32_t initSize = WORD_SIZE+MIN_BLOCK_SIZE+WORD_SIZE;
  uint32_t totalSize;

  void* mem_sbrk_result = mem_sbrk(initSize);
  //  printf("mem_sbrk returned %p\n", mem_sbrk_result);
  if ((uint32_t)mem_sbrk_result == -1) {
    printf("ERROR: mem_sbrk failed in mm_init, returning %p\n", 
           mem_sbrk_result);
    exit(1);
  }

  firstFreeBlock = (Blk_Info*)POINTER_ADD(mem_heap_lo(), WORD_SIZE);

  // Total usable size is full size minus heap-header and heap-footer words
  // NOTE: These are different than the "header" and "footer" of a block!
  // The heap-header is a pointer to the first free block in the free list.
  // The heap-footer is used to keep the data structures consistent (see
  // requestMoreSpace() for more info, but you should be able to ignore it).
  totalSize = initSize - WORD_SIZE - WORD_SIZE;

  // The heap starts with one free block, which we initialize now.
  firstFreeBlock->sizeAndTags = totalSize | TAG_PRECEDING_USED;
  firstFreeBlock->next = NULL;
  firstFreeBlock->prev = NULL;
  // boundary tag
  *((uint32_t*)POINTER_ADD(firstFreeBlock, totalSize - WORD_SIZE)) = totalSize | TAG_PRECEDING_USED;
  
  // Tag "useless" word at end of heap as used.
  // This is the is the heap-footer.
  *((uint32_t*)POINTER_SUB(mem_heap_hi(), WORD_SIZE - 1)) = TAG_USED;

  // set the head of the free list to this new free block.
  FREE_LIST_HEAD = firstFreeBlock;
  return 0;
}

// TOP-LEVEL ALLOCATOR INTERFACE ------------------------------------


/* Allocate a block of size size and return a pointer to it. */
void* mm_malloc (uint32_t size) {
  uint32_t reqSize;
  Blk_Info * ptrFreeBlock = NULL;
  uint32_t blockSize;
  uint32_t precedingBlockUseTag;

  // Zero-size requests get NULL.
  if (size == 0) {
    return NULL;
  }

  // Add one word for the initial size header.
  // Note that we don't need to boundary tag when the block is used!
  size += WORD_SIZE;
  if (size <= MIN_BLOCK_SIZE) {
    // Make sure we allocate enough space for a blockInfo in case we
    // free this block (when we free this block, we'll need to use the
    // next pointer, the prev pointer, and the boundary tag).
    reqSize = MIN_BLOCK_SIZE;
  } else {
    // Round up for correct alignment
    reqSize = ALIGNMENT * ((size + ALIGNMENT - 1) / ALIGNMENT);
  }
  
  // Search the free list for a fit
  ptrFreeBlock = searchFreeList(reqSize);
  
  if (ptrFreeBlock == NULL){
    // If no fit was found, get more heap space 
    requestMoreSpace(reqSize);
    // Search the free list for a fit after growing the heap
    ptrFreeBlock = searchFreeList(reqSize);
  }
  
  // Save the block's preceding used tag
  precedingBlockUseTag = ptrFreeBlock->sizeAndTags & TAG_PRECEDING_USED;

  // Get the size of the block
  blockSize = SIZE(ptrFreeBlock->sizeAndTags);
  
  // Remove the block from the free list
  removeFreeBlock(ptrFreeBlock);
  
  // Split the block if (blockSize - reqSize) >= MIN_BLOCK_SIZE
  if (blockSize - reqSize >= MIN_BLOCK_SIZE) {	
    // Set the block's size and tags
    ptrFreeBlock->sizeAndTags = reqSize | precedingBlockUseTag;
    ptrFreeBlock->sizeAndTags = ptrFreeBlock->sizeAndTags | TAG_USED;
	
    // Set the remainding free block's preceding used tag
    *((uint32_t*) POINTER_ADD(ptrFreeBlock, reqSize)) = (blockSize - reqSize) | TAG_PRECEDING_USED;
    // Set the preceding used tag in boundary tag of the remainding free block
    *((uint32_t*) POINTER_ADD(ptrFreeBlock, blockSize - WORD_SIZE)) = (blockSize - reqSize) | TAG_PRECEDING_USED;
	
    // Insert the remaining free block to the free list
    insertFreeBlock((Blk_Info*) POINTER_ADD(ptrFreeBlock, reqSize));
  } else {
    // Get the pointer to the following block
    Blk_Info* followingBlock = (Blk_Info*) POINTER_ADD(ptrFreeBlock, blockSize);
    // Set the following block's previous used tag
    followingBlock->sizeAndTags = followingBlock->sizeAndTags | TAG_PRECEDING_USED;

    // Set the block's used tag 
    ptrFreeBlock->sizeAndTags =  ptrFreeBlock->sizeAndTags | TAG_USED;
  }
  return ((void*) POINTER_ADD(ptrFreeBlock, WORD_SIZE));
}

void *mm_realloc(void *ptr, uint32_t size)
{
    unsigned int bit0Mask;
    int reqSize;
    int currentTotalSize,currentPayloadSize;
    int adjustedSize;
    int copySize;
    void *oldHeader;
    void *newPtr;
    void *newHeader;

    if (ptr == NULL) return mm_malloc(size);
    if (size == 0) {
	mm_free(ptr);
	return NULL;
    }
  
    oldHeader = (char *)ptr - 4;
    bit0Mask = (*(int *)oldHeader) & 0x1;
    if (bit0Mask != 1 ) /* no warning is given here*/
	return NULL;

    if (size <= 12) reqSize = 16;
    else reqSize = 8 * ((size+4+7)/8);

    currentTotalSize = (*(int *)oldHeader) & (~0x7);
    currentPayloadSize = currentTotalSize - 4;

    ///////
    
    if (currentTotalSize >= reqSize)
    {
        /* now check if the trailer word can fit in or not*/
        if (currentPayloadSize>=(size+4)){
            /* done, no movement necessary*/
            /* create/update header and trailer */
            *(int *)(oldHeader) &= (~0x4);
            *(int *)((char *)oldHeader+currentPayloadSize) = size;
            return ptr;
        }
    }
    /* the realloc cannot fit into the original block */
    /* allocate a new block with larger size than requested and do memcopy*/
    adjustedSize = (int) size * 2 + 4;
 
    newPtr = mm_malloc(adjustedSize);
    newHeader = (char *)newPtr-4;
    *(int *)newHeader &= (~0x4);
    reqSize = (*(int *)newHeader)&(~0x7);
    *(int *)((char *)((char *)newHeader+reqSize) - 4) = size;
  
    if (((*(int *)(oldHeader))&0x4) == 0){ /* already realloc block */
	/* reduce the number of memcopy */
	copySize = *(int *)((char *)((char *)ptr + currentPayloadSize) - 4);
    }
    else copySize = currentPayloadSize; /* at maximum two word overhead*/

    memcpy(newPtr,ptr,copySize);
    mm_free(ptr);
    return newPtr;
}


/* Free the block referenced by ptr. */
void mm_free (void *ptr) {
  uint32_t payloadSize;
  Blk_Info * blockInfo;
  Blk_Info * followingBlock;

  // Get the pointer to the header of this block
  blockInfo = (Blk_Info*) POINTER_SUB(ptr, WORD_SIZE);
  
  // Get the size of the block
  payloadSize = SIZE(blockInfo->sizeAndTags);
  
  // Get the pointer to the following block of this block
  followingBlock = (Blk_Info*) POINTER_ADD(blockInfo, payloadSize);
  // Set the following block's preceding used tag to 0
  followingBlock->sizeAndTags = followingBlock->sizeAndTags & (~TAG_PRECEDING_USED);
  
  // Set the block's used tag to 0
  blockInfo->sizeAndTags = blockInfo->sizeAndTags & (~TAG_USED);
  // Update the block's boundary tag
  *((uint32_t*) POINTER_ADD(blockInfo, payloadSize - WORD_SIZE)) = blockInfo->sizeAndTags;
  
  // Insert the block be freed to the free list
  insertFreeBlock(blockInfo);
  // Coalesce the block with any preceding or following free blocks
  coalesceFreeBlock(blockInfo);
}

// Implement a heap consistency checker as needed.
int mm_check() {
  return 0;
}