/* 
 * mm-implicit.c -  Simple allocator based on implicit free lists, 
 *                  first fit placement, and boundary tag coalescing. 
 *
 * Each block has header and footer of the form:
 * 
 *      31                     3  2  1  0 
 *      -----------------------------------
 *     | s  s  s  s  ... s  s  s  0  0  a/f
 *      ----------------------------------- 
 * 
 * where s are the meaningful size bits and a/f is set 
 * iff the block is allocated. The list has the following form:
 *
 * begin                                                          end
 * heap                                                           heap  
 *  -----------------------------------------------------------------   
 * |  pad   | hdr(8:a) | ftr(8:a) | zero or more usr blks | hdr(8:a) |
 *  -----------------------------------------------------------------
 *          |       prologue      |                       | epilogue |
 *          |         block       |                       | block    |
 *
 * The allocated prologue and epilogue blocks are overhead that
 * eliminate edge conditions during coalescing.
 */
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <memory.h>
#include "mm.h"
#include "memlib.h"
#include <assert.h>

/*********************************************************
 * NOTE TO STUDENTS: Before you do anything else, please
 * provide your team information in the following struct.
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

/////////////////////////////////////////////////////////////////////////////
// Constants and macros
//
// These correspond to the material in Figure 9.43 of the text
// The macros have been turned into C++ inline functions to
// make debugging code easier.
//
/////////////////////////////////////////////////////////////////////////////
#define PREV        0
#define NEXT        4

#define WSIZE       4       /* word size (bytes) */  
#define DSIZE       8       /* doubleword size (bytes) */
#define CHUNKSIZE  (1<<12)  /* initial heap size (bytes) */
#define OVERHEAD    8       /* overhead of header and footer (bytes) */

#define ALIGNMENT 8

#define ALIGN(size) (((size) +  (ALIGNMENT-1)) & ~0x7)

#define SIZE_T_SIZE (ALIGN(sizeof(uint32_t)))

typedef struct header blockHdr;

struct header{
    uint32_t size;
    struct header *next_p;
    struct header *prior_p;
};

static inline int MAX(int x, int y) {
  return x > y ? x : y;
}

//
// Pack a size and allocated bit into a word
// We mask of the "alloc" field to insure only
// the lower bit is used
//
static inline uint32_t PACK(uint32_t size, int alloc) {
  return ((size) | (alloc & 0x1));
}

//
// Read and write a word at address p
//
static inline uint32_t GET(void *p) { return  *(uint32_t *)p; }

//line 102 seg fault
static inline void PUT( void *p, uint32_t val)
{
  *((uint32_t *)p) = val;
}

//
// Read the size and allocated fields from address p
//
static inline uint32_t GET_SIZE( void *p )  { 
  return GET(p) & ~0x7;
}

static inline int GET_ALLOC( void *p  ) {
  return GET(p) & 0x1;
}

//
// Given block ptr bp, compute address of its header and footer
//
static inline void *HDRP(void *bp) {

  return ( (char *)bp) - WSIZE;
}
static inline void *FTRP(void *bp) {
  return ((char *)(bp) + GET_SIZE(HDRP(bp)) - DSIZE);
}

//
// Given block ptr bp, compute address of next and previous blocks
//
static inline void *NEXT_BLKP(void *bp) {
  return  ((char *)(bp) + GET_SIZE(((char *)(bp) - WSIZE)));
}

static inline void* PREV_BLKP(void *bp){
  return  ((char *)(bp) - GET_SIZE(((char *)(bp) - DSIZE)));
}


/////////////////////////////////////////////////////////////////////////////
//
// Global Variables
//

static char *heap_listp;  /* pointer to first block */
static char free_root[8];
static char free_4096[8];


static char *SEGREGATE(uint32_t size){
  if(size < 4096)
    return free_root;
  return free_4096;
}

static void APPEND(void *bp, uint32_t size){
  char *free_list = SEGREGATE(size);
  PUT(bp + PREV, (size_t) free_list);
  PUT(bp + NEXT, GET(free_list + NEXT));
  PUT((void *)(intptr_t)(GET(free_list + NEXT) + PREV), (intptr_t)bp);
  PUT((free_list + NEXT), (size_t) bp);
}

static void DELETE(void *bp){
  PUT((void *)(intptr_t)GET(bp + NEXT) + PREV, GET(bp + PREV));
  PUT((void *)(intptr_t)(GET(bp + PREV) + NEXT), GET(bp + NEXT));
}

static void INIT(){
  PUT(free_root, (intptr_t) free_root);
  PUT(free_root + WSIZE, (intptr_t) free_root);

  APPEND(free_4096, 0);
}

// static void PRINT(){
//   void * walk = (void *)GET(free_root + NEXT);
//   printf("Printing the free list\n");
//   while(walk != free_root){
//     printf("  Pointer: %p\n", walk);
//     printf("     Prev:    %#x\n", GET(walk + PREV));
//     printf("     Next:    %#x\n", GET(walk + NEXT));
//     walk = (void *)GET(walk + NEXT);
//   }
// }

//
// function prototypes for internal helper routines
//
static void *extend_heap(uint32_t words);
static void place(void *bp, uint32_t asize);
static void *find_fit(uint32_t asize);
static void *coalesce(void *bp);
static void printblock(void *bp); 
static void checkblock(void *bp);

//
// mm_init - Initialize the memory manager, min 22 of audio from 11.26.18 
//
/*  austins code

    heap_listp = mem_sbrk(4*WSIZE);
    if(heap_listp == (void *)-1)
        return -1;
    PUT(heap_listp, 0);
    PUT(heap_listp + (1 * WSIZE), PACK(DSIZE, 1)); //prologue header, are now offsetting by 4 from the
    PUT(heap_listp + (2 * WSIZE), PACK(DSIZE, 1)); //prologue footer, are now offsetting by 8 from the
    PUT(heap_listp + (3 * WSIZE), PACK(0, 1));     //epilogue header, offset abother 4 bytes (the word size)
    heap_listp += (2 * WSIZE);
    
    return 0;

*/

int mm_init(void) 
{
  INIT();

  if((heap_listp = mem_sbrk(4 * WSIZE)) == (void *)-1)
    return -1;

  PUT(heap_listp, 0);
  PUT(heap_listp + (1 * WSIZE), PACK(DSIZE, 1));
  PUT(heap_listp + (2 * WSIZE), PACK(DSIZE, 1));
  PUT(heap_listp + (3 * WSIZE), PACK(0, 1));
  heap_listp += (2 * WSIZE);

  char *bp = NULL;
  if((bp = extend_heap(CHUNKSIZE / WSIZE)) == NULL)
    return -1;

  return 0;
}
//
// extend_heap - Extend heap with free block and return its block pointer
//
// gdb mdriver
//break mm_malloc
//(gdb) run
//call mm_checkheap    ...take out these call b4 turning in

/* austins code

    char *bp;
    size_t size;   //perhaps this should be uint32_t

    //allocate an even number of words to maintain alignment
    size = (words%2) ? (words+1) * WSIZE : words * WSIZE
    if((long)bp=mem_sbrk(size)==1)
        return NULL;
    
    //initialize free block header/footer and the epilogue header f
    PUT(HDRP(bp), PACK(size, 0)); //free block header
    PUT(FTRP(bp), PACK(size,0)); //footer
    PUT(HDRP(NEXT_BLKP(bp)), PACK(0,1)); //new epilogue header
    
    return bp;

*/


static void *extend_heap(uint32_t words) 
{
  char *bp;
  uint32_t size;

  size = (words % 2) ? (words + 1) * WSIZE : words * WSIZE;
  if((long)(bp = mem_sbrk(size)) == -1)
    return NULL;

  PUT(HDRP(bp), PACK(size, 0));
  PUT(FTRP(bp), PACK(size, 0));
  PUT(HDRP(NEXT_BLKP(bp)), PACK(0, 1));

  return coalesce(bp);
}
//
// Practice problem 9.8
//
// find_fit - Find a fit for a block with asize bytes 
//
static void *find_fit(uint32_t asize)
{
  char *free_list = SEGREGATE(asize);
  void * bp = (void *)(intptr_t)GET(free_list + NEXT);

  while(bp != free_root){
    if(!GET_ALLOC(HDRP(bp)) && (asize <= GET_SIZE(HDRP(bp)))){
      return bp;
    } else{
      bp = (void *)(intptr_t)GET(bp + NEXT);
    }
  }

  return NULL; /* no fit */
}

// 
// mm_free - Free a block 
//
void mm_free(void *bp)
{
  uint32_t size = GET_SIZE(HDRP(bp));

  PUT(HDRP(bp), PACK(size, 0));
  PUT(FTRP(bp), PACK(size, 0));
  coalesce(bp);
}

//
// coalesce - boundary tag coalescing. Return ptr to coalesced block
//
static void *coalesce(void *bp) 
{
  uint32_t prev_alloc = GET_ALLOC(FTRP(PREV_BLKP(bp)));
  uint32_t next_alloc = GET_ALLOC(HDRP(NEXT_BLKP(bp)));
  uint32_t size = GET_SIZE(HDRP(bp));

  if(prev_alloc && !next_alloc){ 
    size += GET_SIZE(HDRP(NEXT_BLKP(bp)));
    DELETE(NEXT_BLKP(bp));
    PUT(HDRP(bp), PACK(size, 0));
    PUT(FTRP(bp), PACK(size,0));
  }
  else if(!prev_alloc && next_alloc){ 
    size += GET_SIZE(HDRP(PREV_BLKP(bp)));
    DELETE(PREV_BLKP(bp));
    PUT(FTRP(bp), PACK(size, 0));
    PUT(HDRP(PREV_BLKP(bp)), PACK(size, 0));
    bp = PREV_BLKP(bp);
  }
  else if(!prev_alloc && !next_alloc){
    size += GET_SIZE(HDRP(PREV_BLKP(bp))) + GET_SIZE(FTRP(NEXT_BLKP(bp)));
    DELETE(NEXT_BLKP(bp));
    DELETE(PREV_BLKP(bp));
    PUT(HDRP(PREV_BLKP(bp)), PACK(size, 0));
    PUT(FTRP(NEXT_BLKP(bp)), PACK(size, 0));
    bp = PREV_BLKP(bp);
  }

  APPEND(bp, size);
  return bp;
}

//
// mm_malloc - Allocate a block with at least size bytes of payload 
//
void *mm_malloc(uint32_t size) 
{
  uint32_t asize;
  uint32_t extendsize;
  char *bp;

  if(size == 0){
    return NULL;
  }

  if(size <= DSIZE){
    asize = 2 * DSIZE;
  }
  else{
    asize = DSIZE * ((size + DSIZE + (DSIZE - 1)) / DSIZE);
  }

  if((bp = find_fit(asize)) == NULL){
    extendsize = MAX(asize, CHUNKSIZE);
    if((bp = extend_heap(extendsize / WSIZE)) == NULL)
      return NULL;
  }

  DELETE(bp);
  place(bp, asize);
  return bp;
} 

//
//
// Practice problem 9.9
//
// place - Place block of asize bytes at start of free block bp 
//         and split if remainder would be at least minimum block size
//
static void place(void *bp, uint32_t asize)
{
  uint32_t csize = GET_SIZE(HDRP(bp));

  if((csize - asize) >= (2 * DSIZE)){
    PUT(HDRP(bp), PACK(asize, 1));
    PUT(FTRP(bp), PACK(asize, 1));
    bp = NEXT_BLKP(bp);
    PUT(HDRP(bp), PACK(csize-asize, 0));
    PUT(FTRP(bp), PACK(csize-asize, 0));
    coalesce(bp);
  }
  else{
    PUT(HDRP(bp), PACK(csize, 1));
    PUT(FTRP(bp), PACK(csize, 1));
  }
}


//
// mm_realloc -- implemented for you
//
void *mm_realloc(void *ptr, uint32_t size)
{
  void *newp;
  uint32_t copySize;

  if(ptr == NULL){
    return mm_malloc(size);
  }
  if(size == 0){
    free(ptr);
    return NULL;
  }


  newp = mm_malloc(size);
  if (newp == NULL) {
    printf("ERROR: mm_malloc failed in mm_realloc\n");
    exit(1);
  }

  copySize = GET_SIZE(HDRP(ptr));

  if (size < copySize) {
    copySize = size;
  }
  memcpy(newp, ptr, copySize);
  mm_free(ptr);
  return newp;
}

//
// mm_checkheap - Check the heap for consistency 
//
void mm_checkheap(int verbose) 
{
  //
  // This provided implementation assumes you're using the structure
  // of the sample solution in the text. If not, omit this code
  // and provide your own mm_checkheap
  //
  void *bp = heap_listp;
  
  if (verbose) {
    printf("Heap (%p):\n", heap_listp);
  }

  if ((GET_SIZE(HDRP(heap_listp)) != DSIZE) || !GET_ALLOC(HDRP(heap_listp))) {
	printf("Bad prologue header\n");
  }
  checkblock(heap_listp);

  for (bp = heap_listp; GET_SIZE(HDRP(bp)) > 0; bp = NEXT_BLKP(bp)) {
    if (verbose)  {
      printblock(bp);
    }
    checkblock(bp);
  }
     
  if (verbose) {
    printblock(bp);
  }

  if ((GET_SIZE(HDRP(bp)) != 0) || !(GET_ALLOC(HDRP(bp)))) {
    printf("Bad epilogue header\n");
  }
}

static void printblock(void *bp) 
{
  uint32_t hsize, halloc, fsize, falloc;

  hsize = GET_SIZE(HDRP(bp));
  halloc = GET_ALLOC(HDRP(bp));  
  fsize = GET_SIZE(FTRP(bp));
  falloc = GET_ALLOC(FTRP(bp));  
    
  if (hsize == 0) {
    printf("%p: EOL\n", bp);
    return;
  }

  printf("%p: header: [%d:%c] footer: [%d:%c]\n",
	 bp, 
	 (int) hsize, (halloc ? 'a' : 'f'), 
	 (int) fsize, (falloc ? 'a' : 'f')); 
}

//below changed size_t to uint32_t
static void checkblock(void *bp) 
{
  if ((intptr_t)bp % 8) {
    printf("Error: %p is not doubleword aligned\n", bp);
  }
  if (GET(HDRP(bp)) != GET(FTRP(bp))) {
    printf("Error: header does not match footer\n");
  }
}
