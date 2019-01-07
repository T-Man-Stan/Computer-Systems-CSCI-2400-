/*
 * mm-naive.c - The fastest, least memory-efficient malloc package.
 * 
 * In this naive approach, a block is allocated by simply incrementing
 * the brk pointer.  A block is pure payload. There are no headers or
 * footers.  Blocks are never coalesced or reused. Realloc is
 * implemented directly using mm_malloc and mm_free.
 *
 * NOTE TO STUDENTS: Replace this header comment with your own header
 * comment that gives a high level description of your solution.
 */
#include <stdio.h>
#include <stdlib.h>
#include <assert.h>
#include <unistd.h>
#include <string.h>
#include <memory.h>
#include "mm.h"
#include "memlib.h"

/*********************************************************
 * NOTE TO STUDENTS: Before you do anything else, please
 * provide your identifying information in the following struct.
 ********************************************************/
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

/* single word (4) or double word (8) alignment */
#define ALIGNMENT 8

/* rounds up to the nearest multiple of ALIGNMENT */
#define ALIGN(size) (((size) + (ALIGNMENT-1)) & ~0x7)

#define SIZE_T_SIZE (ALIGN(sizeof(uint32_t)))

/* Declare header structure */
typedef struct block hblock;

struct block {
    uint32_t header;
    hblock *succ_p;
    hblock *pred_p;
    uint32_t footer;
};

#define WSIZE      4
#define DSIZE      8
#define CHUNKSIZE  (1<<12)  /* initial heap size (bytes) */
#define OVERHEAD    8       /* overhead of header and footer (bytes) */

static inline int MAX(int x, int y) {
  return x > y ? x : y;
}


/* Basic constants and macros */
#define HSIZE      ALIGN(sizeof(hblock)) /* The minimum size of a block */

/* Pack a size and allocated bit into a word */
#define PACK(size, alloc) ((size) | (alloc))

/* Read and write a word at address p */
#define GET(p)        (*(uint32_t *)(p))
#define PUT(p, val)   (*(uint32_t *)(p) = (val))

/* Read the size and allocated fields from address p */
#define GET_SIZE(p)  (GET(p) & ~0x1)
#define GET_ALLOC(p) (GET(p) & 0x1)

/* Given block ptr bp, compute the address of its header and footer */
#define HDRP(bp)     ((char *)(bp))
#define FTRP(bp)     ((char *)(bp) + GET_SIZE(bp))

/* Given a block ptr bp, compute the address of next and previous payload blocks */
#define NEXT_BLKP(bp)    ((char *)(bp) + GET_SIZE(bp))
#define PREV_BLKP(bp)    ((char *)(bp) - GET_SIZE(bp))

/* Declarations */
static void *find_free(uint32_t size);
static void *coalesce(hblock *p);
static void place(hblock *p, uint32_t newsize);
void print_heap();
static void remove_free_block(hblock *bp);

/* Private global variables */
static char *p heap_listp /*ptr to the first block*/
static void *rover
static hblock char *p;         /* Points to the starting block at all times */

/* 
 * mm_init - Initialize the malloc package, and return 0 if successful and -1 otherwise.
 */
int mm_init(void)
{
    if((heap_listp = mem_sbrl(4*WSIZE)) == (void *)-1)
        return 0;
    PUT(heap_list, 0);
    PUT(heap_list + (1*WSIZE), PACK(DSIZE, 1));
    PUT(heap_list + (2*WSIZE), PACK(DSIZE, 1));
    PUT(heap_list + (3*WSIZE), PACK(0, 1));
    heap_listp += (2*WSIZE);
    

    if(extend_heap(CHUNKSIZE/WSIZE)==NULL)
        return -1;
    
    return 0;
}

/* 
 * mm_malloc - Allocate a block by incrementing the brk pointer.
 *     Always allocate a block whose size is a multiple of the alignment.
 */

static void *extend_heap(uint32 words)
{
    char *bp
        uint32_t size;
        
        size = (words % 2) ? (words+1) * WSIZE : words * WSIZE;
        if ((long)(bp = mem_sbrk(size)) == -1)
            return NULL;
    PUT(HDRP(bp), PACK(size, 0));
    PUT(FTRP(bp), PACK(size, 0));
    PUT(HDRP(NEXT_BLKP(bp)), PACK(0, 1));
    
    return coalesce(bp);
}

void *mm_malloc(uint32_t size)
{
    /* Ignore spurious requests */
    if (size < 1)
        return NULL;

    /* The size of the new block is equal to the size of the header, plus
     * the size of the payload
     */
    int newsize = ALIGN(size + HSIZE);
    
    /* Try to find a free block that is large enough */
    hblock *bp = (hblock *) find_free(newsize);
   
    /* If a large enough free block was not found, then coalesce
     * the existing free blocks */ 

    /* After coalsecing, if a large enough free block cannot be found, then
     * extend the heap with a free block */
    if (bp == NULL) { 
        bp = mem_sbrk(newsize);
        if ((long)(bp) == -1)
            return NULL;
        else {
            bp->header = newsize | 0x1;
            bp->footer = bp->header;
        }
    }
    else {
        /* Otherwise, a free block of the appropriate size was found. Place
         * the block */
        place(bp, newsize); 
    }

    // Return a pointer to the payload
    return (char *) bp + HSIZE;
}

/*
 * mm_free - Freeing a block does nothing.
 */
void mm_free(void *ptr)
{
    /* Get the pointer to the allocated block */
    hblock *bp = ptr - HSIZE; 
    //hblock *cbp; // stores a pointer to the coalesced block

    //cbp = (hblock *) coalesce(bp);

    /* Modify the allocated bit for the header and footer */ 
    bp->header &= ~1;
    bp->footer = bp->header;

    /* Set up the doubly linked explicit free list */
    bp->succ_p = p->succ_p;
    bp->pred_p = p;
    p->succ_p = bp;
    bp->succ_p->pred_p = bp;
   
    return;
}

/*
 * mm_realloc - Implemented simply in terms of mm_malloc and mm_free
 */
void *mm_realloc(void *ptr, uint32_t size)
{
    return NULL;
}

/** BEGIN HELPER FUNCTIONS **/

/*
 * Attempts to find the right size of free block in the free list. The free list is 
 * implemented as an explicit list, which is simply a doubly linked list.
 */
static void *find_free(uint32_t size)
{
    /* Iterate over each of the blocks in the free list until a block
     * of the appropriate size is found. If the block wraps back around
     * to the prologue, then a free block wasn't found.
     */ 
    hblock *bp;
    for(bp = p->succ_p; bp != p && bp->header < size; bp = bp->succ_p) {
    }

    /* If the free list wrapped back around, then there were no free spots */
    if (bp == p)
        return NULL;
    else
    /* Otherwise return the pointer to the free block */
        return bp; 
}

static void *coalesce(hblock *bp)
{
    uint32_t prev_alloc = GET_ALLOC(FTRP(PREV_BLKP(bp)));
    uint32_t next_alloc = GET_ALLOC(HDRP(NEXT_BLKP(bp)));
    uint32_t size = GET_SIZE(HDRP(bp));  //added (HDRP(bp))

    if (prev_alloc && next_alloc) {            /* Case 1 */
      //      fprintf(stderr,"Case 1: prev bsize is %d, next size is %d\n", 
      //	      GET_SIZE(HDRP(PREV_BLKP(bp))), 	    GET_SIZE(FTRP(NEXT_BLKP(bp))) );

	return bp;
    }

    else if (prev_alloc && !next_alloc) {      /* Case 2 */
      //      fprintf(stderr,"Case 2: prev bsize is %d, next size is %d\n", 
      //	      GET_SIZE(HDRP(PREV_BLKP(bp))), 	    GET_SIZE(FTRP(NEXT_BLKP(bp))) );


	size += GET_SIZE(HDRP(NEXT_BLKP(bp)));
	PUT(HDRP(bp), PACK(size, 0));
	PUT(FTRP(bp), PACK(size,0));
    }

    else if (!prev_alloc && next_alloc) {      /* Case 3 */
      //      fprintf(stderr,"Case 3: prev bsize is %d, next size is %d\n", 
      //	      GET_SIZE(HDRP(PREV_BLKP(bp))), 	    GET_SIZE(FTRP(NEXT_BLKP(bp))) );

	size += GET_SIZE(HDRP(PREV_BLKP(bp)));
	PUT(FTRP(bp), PACK(size, 0));
	PUT(HDRP(PREV_BLKP(bp)), PACK(size, 0));
	bp = PREV_BLKP(bp);
    }

    else {                                     /* Case 4 */
      //      fprintf(stderr,"Case 4: prev bsize is %d, next size is %d\n", 
      //	      GET_SIZE(HDRP(PREV_BLKP(bp))), 	    GET_SIZE(FTRP(NEXT_BLKP(bp))) );


	size += GET_SIZE(HDRP(PREV_BLKP(bp))) + 
	    GET_SIZE(FTRP(NEXT_BLKP(bp)));
	PUT(HDRP(PREV_BLKP(bp)), PACK(size, 0));
	PUT(FTRP(NEXT_BLKP(bp)), PACK(size, 0));
	bp = PREV_BLKP(bp);
    }

#ifdef NEXT_FIT
    /* Make sure the rover isn't pointing into the free block */
    /* that we just coalesced */
    if ((rover > (char *)bp) && (rover < NEXT_BLKP(bp)))  {
	rover = bp;
    }
#endif

    return bp;
}

static void place(hblock *bp, uint32_t newsize)
{  
    uint32_t csize = GET_SIZE(bp->header);

    if ((csize - newsize) >= 24) {
        bp->header = newsize | 0x1;
        bp->footer = bp->header;
        remove_free_block(bp);
        bp = (hblock *) NEXT_BLKP(bp);
        bp->header = (csize-newsize) | 0x0;
        bp->footer = bp->header; 
        coalesce(bp);
    }


    else {
        bp->header = csize | 0x1;
        bp->footer = bp->header;
        remove_free_block(bp);
    }
    /* Set the allocated bit of the header and footer */
    //bp->header |= 0x1;
    //bp->footer = bp->header;

    /* Set up the link for the free list */  
    //remove_free_block(bp);

    return;
}

void print_heap()
{
    hblock *bp = mem_heap_lo();
    while(bp < (hblock *) mem_heap_hi()) {
        printf("%s block at %p, size %d\n", 
               GET_ALLOC(bp) ? "allocated":"free", bp, GET_SIZE(bp));
        bp = (hblock *) NEXT_BLKP(bp); 
    }
}

static void remove_free_block(hblock *bp)
{
    bp->pred_p->succ_p = bp->succ_p;
    bp->succ_p->pred_p = bp->pred_p;
}