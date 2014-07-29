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

#include "mm.h"
#include "memlib.h"


/*********************************************************
 * NOTE TO STUDENTS: Before you do anything else, please
 * provide your team information in the following struct.
 ********************************************************/
team_t team = {
    /* Team name */
    "5130379013",
    /* First member's full name */
    "Zhu Chaojie",
    /* First member's email address */
    "zackszcj@sjtu.edu.cn",
    /* Second member's full name (leave blank if none) */
    "",
    /* Second member's email address (leave blank if none) */
    ""
};

/* single word (4) or double word (8) alignment */
#define ALIGNMENT 8

/* rounds up to the nearest multiple of ALIGNMENT */
#define ALIGN(size) (((size) + (ALIGNMENT-1)) & ~0x7)


#define SIZE_T_SIZE (ALIGN(sizeof(size_t)))

/* Basic constants and macros */
#define WSIZE 4 /* word size (bytes) */
#define DSIZE 8 /* double word size (bytes) */
#define CHUNKSIZE (1 << 13) /* Extend heap by this amount (bytes) */
#define OVERHEAD 2*WSIZE

#define MAX(x, y) ((x) > (y)? (x) : (y))
/* Pack a size and allocated bit into a word */
#define PACK(size, alloc) ((size) | (alloc))
/* Read and write a word at address p */
#define GET(p) (*(unsigned int *)(p))
#define PUT(p, val) (*(unsigned int *)(p) = (val))

/* Read the size and allocated fields from address p */
#define GET_SIZE(p) (GET(p) & ~0x7)
#define GET_ALLOC(p) (GET(p) & 0x1)
/* Given block ptr bp, compute address of its header and footer */
#define HDRP(bp) ((char *)(bp) - WSIZE)
#define FTRP(bp) ((char *)(bp) + GET_SIZE(HDRP(bp)) - DSIZE)

/* Given block ptr bp, compute address of next and previous blocks */
#define NEXT_BLKP(bp) ((char *)(bp) + GET_SIZE(((char *)(bp) - WSIZE)))
#define PREV_BLKP(bp) ((void *)(bp) - GET_SIZE(((void *)(bp) - DSIZE)))

static void *heap_listp = 0; // the head of the heep
static void *lastFree = 0; // the end of the list of free blocks

static void *prevFree(void *ptr) {
    // get the previous free block in the free list
    return (void *)(GET(HDRP(ptr) + WSIZE));
}


static void *nextFree(void *ptr) {
    // get the next free block in the free list
    return (void *)(GET(FTRP(ptr) - WSIZE));
}


static void deleteNode(void *ptr) {
    // delete the item in the free list
    void *prevB = prevFree(ptr), *nextB = nextFree(ptr);
    if (prevB && nextB){
        // change the foot of previous one and the head of next one
        PUT((size_t)FTRP(prevB) - WSIZE, nextB);
        PUT((size_t)HDRP(nextB) + WSIZE, prevB);
    }
    else if (prevB){
        // no next then set lastFree
        PUT((size_t)FTRP(prevB) - WSIZE, 0);
        lastFree = prevB;
    }
    else if (nextB){
        PUT((size_t)HDRP(nextB) + WSIZE, 0);
    }
    else
        lastFree = 0; // have delete all
}


static void *coalesce(void *bp) {
    size_t prev_alloc = GET_ALLOC(FTRP(PREV_BLKP(bp)));
    size_t next_alloc = GET_ALLOC(HDRP(NEXT_BLKP(bp)));
    size_t size = GET_SIZE(HDRP(bp));

    if (prev_alloc && next_alloc) { /* Case 1 */
        return bp;
    }
    else if (prev_alloc && !next_alloc) { /* Case 2 */
        // delete the behind one and change the links of the front one in the free list
        size += GET_SIZE(HDRP(NEXT_BLKP(bp)));
        deleteNode(NEXT_BLKP(bp));
        PUT(FTRP(NEXT_BLKP(bp)) - WSIZE, GET(FTRP(bp) - WSIZE));
        PUT(HDRP(bp), PACK(size, 0));
        PUT(FTRP(bp), PACK(size, 0));
        return(bp);
    }
    else if (!prev_alloc && next_alloc) { /* Case 3 */
        // delete the behind one and change the links of the front one in the free list
        size += GET_SIZE(HDRP(PREV_BLKP(bp)));
        deleteNode(bp);
        PUT(FTRP(bp) - WSIZE, GET(FTRP(PREV_BLKP(bp)) - WSIZE));
        PUT(FTRP(bp), PACK(size, 0));
        PUT(HDRP(PREV_BLKP(bp)), PACK(size, 0));
        return(PREV_BLKP(bp));
    }
    else { /* Case 4 */
        // delete the behind and mid one and change the links of the front one in the free list
        size += GET_SIZE(HDRP(PREV_BLKP(bp))) + GET_SIZE(FTRP(NEXT_BLKP(bp)));
        deleteNode(NEXT_BLKP(bp));
        deleteNode(bp);
        PUT(FTRP(NEXT_BLKP(bp)) - WSIZE, GET(FTRP(PREV_BLKP(bp)) - WSIZE));
        PUT(FTRP(NEXT_BLKP(bp)), PACK(size, 0));
        PUT(HDRP(PREV_BLKP(bp)), PACK(size, 0));
        return(PREV_BLKP(bp));
    }
}

static void *find_fit(size_t asize)
{
    void *bp;

    /* first fit search */
    for (bp = lastFree; bp && GET_SIZE(HDRP(bp)) > 0 ; bp = prevFree(bp) ) {
        if (!GET_ALLOC(HDRP(bp)) && (asize <= GET_SIZE(HDRP(bp)))) {
            return bp;
        }   
    }
    return NULL;
}

static void place(void *bp, size_t asize){
    size_t csize = GET_SIZE(HDRP(bp));
    // delete first then add new
    deleteNode(bp);
    if ((csize - asize) >= (DSIZE + OVERHEAD)){
        PUT(HDRP(bp), PACK(asize, 1));
        PUT(FTRP(bp), PACK(asize, 1));
        bp = NEXT_BLKP(bp);
        PUT(HDRP(bp), PACK(csize-asize, 0));
        PUT(FTRP(bp), PACK(csize-asize, 0));
        // add to the tail of the list
        PUT(HDRP(bp) + WSIZE, lastFree);
        PUT(FTRP(bp) - WSIZE, 0);
        if (lastFree) {
            // add link of the original tail
            PUT(FTRP(lastFree) - WSIZE, bp);
        }
        lastFree = bp;
    }
    else{
        // just place
        PUT(HDRP(bp), PACK(csize, 1));
        PUT(FTRP(bp), PACK(csize, 1));
    }
}

/*
 * mm_init - initialize the malloc package.
 */

static void *extend_heap(size_t words){
    char *bp;
    size_t size;
    /* Allocate an even number of words to maintain alignment */
    size = (words % 2) ? (words+1) * WSIZE : words * WSIZE;

    if ((long)(bp = mem_sbrk(size)) == -1)
        return NULL;

    /* Initialize free block header/footer and the epilogue header */
    PUT(HDRP(bp), PACK(size, 0));       /* free block header */
    PUT((size_t)HDRP(bp) + WSIZE, lastFree);    // link the last free block
    PUT(FTRP(bp), PACK(size, 0));       /* free block footer */
    // add new extended blocks to the tail
    PUT((size_t)FTRP(bp) - WSIZE, 0);
    if (lastFree){
        PUT((size_t)FTRP(lastFree) - WSIZE, bp);
    }
    lastFree = bp;                      // set as the last free block
    PUT(HDRP(NEXT_BLKP(bp)), PACK(0, 1)); /* new epilogue header */

    /* Coalesce if the previous block was free */
    return coalesce(bp);
}

int mm_init(void)
{
    /* create the initial empty heap */
    lastFree = 0;
    if ((heap_listp = mem_sbrk(4*WSIZE)) == (void *) -1)
        return -1;
    PUT(heap_listp, 0);                      /* alignment padding */
    PUT(heap_listp+(1*WSIZE), PACK(DSIZE, 1));  /* prologue header */
    PUT(heap_listp+(2*WSIZE), PACK(DSIZE, 1));  /* prologue footer */
    PUT(heap_listp+(3*WSIZE), PACK(0, 1));           /* epilogue header */
    heap_listp += (2*WSIZE);

    /* Extend the empty heap with a free block of CHUNKSIZE bytes */
    if (extend_heap(CHUNKSIZE/WSIZE) == NULL)
        return -1;
    return 0;
}

/*
 * mm_malloc - Allocate a block by incrementing the brk pointer.
 *     Always allocate a block whose size is a multiple of the alignment.
 */
void *mm_malloc(size_t size)
{
    // mm_check();
    size_t asize; /* adjusted block size */
    size_t extendSize; /* amount to extend heap if no fit */
    char *bp;

    /* Ignore spurious requests */
    if (size <= 0)
        return NULL;

    /* Adjust block size to include overhead and alignment reqs. */
    if (size <= DSIZE)
        asize = DSIZE + OVERHEAD;
    else
        asize = DSIZE * ((size + (OVERHEAD) + (DSIZE-1)) / DSIZE);

    /* Search the free list for a fit */
    if ((bp = find_fit(asize)) != NULL) {
        place(bp, asize);

        return bp;
    }

    /* No fit found. Get more memory and place the block */
    extendSize = MAX(asize, CHUNKSIZE) ;
    if ((bp = extend_heap(extendSize/WSIZE)) == NULL)
        return NULL;
    place(bp, asize);

    return bp;
}

/*
 * mm_free - Freeing a block does nothing.
 */
void mm_free(void *ptr)
{
    size_t size = GET_SIZE(HDRP(ptr));
    PUT(HDRP(ptr), PACK(size, 0));
    PUT(FTRP(ptr), PACK(size, 0));
    // add this new free block to the tail
    PUT(HDRP(ptr) + WSIZE, lastFree);
    PUT(FTRP(ptr) - WSIZE, 0);
    if (lastFree) {
        PUT((size_t)FTRP(lastFree) - WSIZE, ptr);
    }
    lastFree = ptr;
    coalesce(ptr);
}

/*
 * mm_realloc - Implemented simply in terms of mm_malloc and mm_free
 */
void *mm_realloc(void *ptr, size_t size)
{

    void *newptr = NEXT_BLKP(ptr);
    size_t copySize;

    if (size <= DSIZE)
        size = DSIZE + OVERHEAD;
    else
        size = DSIZE * ((size + (OVERHEAD) + (DSIZE-1)) / DSIZE);

    if (!GET_ALLOC(HDRP(NEXT_BLKP(ptr)))) {
        if (GET_SIZE(HDRP(ptr)) + GET_SIZE(HDRP(newptr)) > size + 2 * DSIZE) {
            // if the next block is free and the size is suitable
            int newSize = GET_SIZE(HDRP(ptr)) + GET_SIZE(HDRP(NEXT_BLKP(ptr))) - size; // calc new size
            void *prev = prevFree(newptr), *next = nextFree(newptr); // record the current status
            PUT(HDRP(ptr), PACK(size, 1));
            PUT(FTRP(ptr), PACK(size, 1));
            ptr = NEXT_BLKP(ptr);
            PUT(HDRP(ptr), PACK(newSize, 0));
            PUT(FTRP(ptr), PACK(newSize, 0));
            PUT(HDRP(ptr) + WSIZE, prev); // change the place of prev link
            // change the links arround
            if (prev) {
                PUT(FTRP(prev) - WSIZE, ptr);
            }
            if (next) {
                PUT(HDRP(next) + WSIZE, ptr);
            }
            else
                lastFree = ptr;
    
            return PREV_BLKP(ptr);
        }
    }
    // if no free blocks behind, malloc new area and free this area
    newptr = mm_malloc(size);

    if (newptr == NULL)
        return NULL;

    if (ptr == NULL)
        return newptr;

    if (size != 0) {
        copySize = GET_SIZE(HDRP(ptr));
        if (size < copySize)
            copySize = size;
        memcpy(newptr, ptr, copySize);
    }
    else {
        newptr = 0;
    }

    mm_free(ptr);
    
    return newptr;
}


void mm_check() {
    void *ptr;
    int freeNumA = 0, freeNumB = 0;
    for (ptr = heap_listp; GET_SIZE(HDRP(ptr)) > 0; ptr = NEXT_BLKP(ptr)){
        if (GET_ALLOC(HDRP(ptr)) == 0)
            freeNumA++; // record the number of free blocks in the heap
    }
    for (ptr = lastFree; ptr && GET_SIZE(HDRP(ptr)) > 0 ; ptr = prevFree(ptr)) {
        if (GET_ALLOC(HDRP(ptr)) == 0)
            freeNumB++; // record the number of free blocks in the list
        else{
            // un-free block in the free list
            printf("Error: The block in free list is not free.\n");
            return;
        }
    }
    if (freeNumA != freeNumB) {
        // numbers are not equal
        printf("Error: Not all free block in free list.\n");
    }
}