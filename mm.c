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
    "07-7team",
    /* First member's full name */
    "Ahram",
    /* First member's email address */
    "helloahram",
    /* Second member's full name (leave blank if none) */
    "YongJae",
    /* Second member's email address (leave blank if none) */
    "Dohyun"};

/* single word (4) or double word (8) alignment */
#define ALIGNMENT 8

/* rounds up to the nearest multiple of ALIGNMENT */
#define ALIGN(size) (((size) + (ALIGNMENT - 1)) & ~0x7)

#define SIZE_T_SIZE (ALIGN(sizeof(size_t)))
static char *heap_listp = NULL;

/*
 * mm_init - initialize the malloc package.
 */
int mm_init(void)
{
    /* Create the initial empty heap */
    /* heap_listp 영역 할당 4 * WSIZE 만큼 */
    if ((heap_listp = mem_sbrk(4 * WSIZE)) == (void *)-1)
        return -1;
    PUT(heap_listp, 0);                            /* Alignment Padding */
    PUT(heap_listp + (1 * WSIZE), PACK(DSIZE, 1)); /* Prologue Header */
    PUT(heap_listp + (2 + WSIZE), PACK(DSIZE, 1)); /* Prologue Footer */
    PUT(heap_listp + (3 * WSIZE), PACK(0, 1));     /* Epilogue Header */
    heap_listp += (2 * WSIZE);

    /* Extend the empty heap with a free block of CHUNKSIZE bytes */
    if (extend_heap(CHUNKSIZE / WSIZE) == NULL)
        return -1;

    return 0;
}

/* 추가
 * extend_heap
 */
/* Word, 4 byte 단위로 매개 변수를 받는다, 확장할 힙의 '칸 수' */
static void *extend_heap(size_t words)
{
    char *bp; // 바이트 단위 연산을 위해 char * 사용
    size_t size;

    size = (words % 2) ? (words + 1) * WSIZE : words * WSIZE;
    // mem_sbrk 호출하여 size 만큼 확장
    if ((long)(bp = mem_sbrk(size)) == -1)
        return NULL;

    /* Initialize free block header/ footer*/
    PUT(HDRP(bp), PACK(size, 0));
    PUT(FTRP(bp), PACK(size, 0));
    /* Set Epilogue Block */
    PUT(HDRP(NEXT_BLKP(bp)), PACK(0, 1));

    // 다 만들고 coalesce() 함수를 호출해 연속된 비할당 블록 연결
    return coalesce(bp);
}

/* 추가
 * coalesce 가용 가능한 메모리가 연속되어 있을 때, 경우에 맞게 합병하는 함수
 */
static void *coalesce(void *bp)
{
    size_t prev_alloc = GET_ALLOC(FTRP(PREV_BLKP(bp)));
    size_t next_alloc = GET_ALLOC(HDRP(NEXT_BLKP(bp)));
    size_t size = GET_SIZE(HDRP(bp));

    /* Size 1 */
    if (prev_alloc && next_alloc)
        return bp;
    /* Size 2 */
    else if (prev_alloc && !next_alloc)
    {
        size += GET_SIZE(HDRP(NEXT_BLKP(bp)));
        PUT(HDRP(bp), PACK(size, 0));
        PUT(FTRP(bp), PACK(size, 0));
    }
    /* Size 3 */
    else if (!prev_alloc && next_alloc)
    {
        size += GET_SIZE(HDRP(PREV_BLKP(bp)));
        PUT(FTRP(bp), PACK(size, 0));
        PUT(HDRP(PREV_BLKP(bp)), PACK(size, 0));
        bp = PREV_BLKP(bp);
    }
    /* Size 4*/
    else
    {
        size += GET_SIZE(HDRP(PREV_BLKP(bp))) + GET_SIZE(HDRP(NEXT_BLKP(bp)));
        PUT(HDRP(PREV_BLKP(bp)), PACK(size, 0));
        PUT(FTRP(PREV_BLKP(bp)), PACK(size, 0));
        bp = PREV_BLKP(bp);
    }

    return bp;
}

/*
 * place - Place Block of asize bytes at start of free block up
 * and split if remainder would be at least block size
 */
static void place(void *bp, size_t asize)
{
    size_t csize = GET_SIZE(HDRP(bp));
    // asize - 메모리 할당에서 요청된 블록 크기
    // csize - 현재 블록의 크기

    if ((csize - asize) >= (2 * DSIZE))
    { // 남은 공간이 충분하다면, 블록을 쪼갠다
        PUT(HDRP(bp), PACK(asize, 1));
        PUT(FTRP(bp), PACK(asize, 1));
        bp = NEXT_BLKP(bp);

        PUT(HDRP(bp), PACK(csize - asize, 0));
        PUT(FTRP(bp), PACK(csize - asize, 0));
    }
    else
    { // 남은 공간이 충분하지 않다면 csize 만큼만 할당
        PUT(HDRP(bp), PACK(csize, 1));
        PUT(FTRP(bp), PACK(csize, 1));
    }
}

/*
 * mm_malloc - Allocate a block by incrementing the brk pointer.
 *     Always allocate a block whose size is a multiple of the alignment.
 */
void *mm_malloc(size_t size)
{                      // size - 사용자가 요청한 메모리 크기
    size_t asize;      // 정렬 요구사항에 맞춘 블록 크기
    size_t extendsize; // 힙을 확장할 때 필요한 크기
    char *bp;

    /* Ignore spurious requests */
    if (size == 0)
        return NULL;

    /* Adjust block size to include overhead and alignment reqs. */
    if (size <= DSIZE)
        asize = 2 * DSIZE; // 2 * DSIZE 는 최소 블록 크기
    else
        // 8 bytes 정렬을 만족하는 크기로 올림하는 계산 수행
        asize = DSIZE * ((size + (DSIZE) + (DSIZE)) / DSIZE);

    /* Search the free list for a fit */
    if ((bp = find_fit(asize)) != NULL)
    {
        place(bp, asize);
        return bp;
    }

    /* No fit found, Get more memory and place the block */
    extendsize = MAX(asize, CHUNKSIZE);
    if ((bp = extend_heap(extendsize / WSIZE)) == NULL)
        return NULL;
    place(bp, asize);
    return bp;
}

/* Fit 변경할 때 define + ifdef 로 바꾸면 편하대! */

/*
 * First-fit Search
 */
static void *find_fit(size_t asize)
{
    void *bp;

    for (bp = heap_listp; GET_SIZE(HDRP(bp)); bp = NEXT_BLKP(bp))
    {
        if (!GET_ALLOC(HDRP(bp)) && (asize <= GET_SIZE(HDRP(bp))))
            return bp;
    }
    return NULL; // No fit
}

/*
 * mm_free - Freeing a block does nothing.
 */
void mm_free(void *ptr)
{
    // 해당 포인터의 사이즈 가져오기
    size_t size = GET_SIZE(HDRP(ptr));
    // header/ Footer 에 할당 상태 0 으로 PUT 연산
    PUT(HDRP(ptr), PACK(size, 0));
    PUT(FTRP(ptr), PACK(size, 0));
    // coalesce(ptr) 호출하여 연속된 비어 있는 공간 합병
    coalesce(ptr);
}

/*
 * mm_realloc - Implemented simply in terms of mm_malloc and mm_free
 */
void *mm_realloc(void *ptr, size_t size)
{
    void *oldptr = ptr;
    void *newptr;
    size_t copySize;

    newptr = mm_malloc(size);
    if (newptr == NULL)
        return NULL;
    copySize = *(size_t *)((char *)oldptr - SIZE_T_SIZE);
    if (size < copySize)
        copySize = size;
    memcpy(newptr, oldptr, copySize);
    mm_free(oldptr);
    return newptr;
}
