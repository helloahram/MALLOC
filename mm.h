#include <stdio.h>

extern int mm_init(void);
extern void *mm_malloc(size_t size);
extern void mm_free(void *ptr);
extern void *mm_realloc(void *ptr, size_t size);

/*
 * Students work in teams of one or two.  Teams enter their team name,
 * personal names and login IDs in a struct of this
 * type in their bits.c file.
 */

typedef struct
{
    char *teamname; /* ID1+ID2 or ID1 */
    char *name1;    /* full name of first member */
    char *id1;      /* login ID of first member */
    char *name2;    /* full name of second member (if any) */
    char *id2;      /* login ID of second member */
} team_t;

extern team_t team;

/* Double Word (8) Alignment */
#define ALIGNMENT 8

/* Rounds up to the nearest Multiple of ALIGNMENT */
#define ALIGN(size) (((size) + (ALIGNMENT - 1)) & ~0x07)

// 컴퓨터 사양 32bit or 64bit 에 따라서 size_t 가 달라짐
#define SIZE_T_SIZE (ALIGN(sizeof(size_t)))

/* Basic constants and macros */
#define WSIZE 4             /* Word and Header/ Footer Size (bytes) */
#define DSIZE 8             /* Double Word Size (bytes) */
#define CHUNKSIZE (1 << 12) /* Extend heap by this amount (bytes) */

#define MAX(x, y) ((x) > (y) ? (x) : (y)) // 두 값 중 큰 값 반환

/* Pack a size and allocated bit into a word */
// 사이즈와 할당 비트를 하나의 워드에 패킹
#define PACK(size, alloc) ((size) | (alloc))

/* Read and Write a word at address p */
#define GET(p) (*(unsigned int *)(p))              // 주소 p에서 값을 읽음
#define PUT(p, val) (*(unsigned int *)(p) = (val)) // 주소 p에 값을 씀

/* Read the size and Allocated field from address p */
#define GET_SIZE(p) (GET(p) & ~0x7) // 하위 3비트를 제외한 블록 크기 추출
#define GET_ALLOC(p) (GET(p) & 0x1) // 블록의 할당 여부 추출

/* Given Block ptr bp, coumpute address of its header and footer */
#define HDRP(bp) ((char *)(bp) - WSIZE)                      // 헤더의 주소
#define FTRP(bp) ((char *)(bp) + GET_SIZE(HDRP(bp)) - DSIZE) // 푸터의 주소

/* Given Block ptr bp, compute address of next and previous blocks */
#define NEXT_BLKP(bp) ((char *)(bp) + GET_SIZE(((char *)(bp) - WSIZE)))
#define PREV_BLKP(bp) ((char *)(bp) - GET_SIZE(((char *)(bp) - DSIZE)))