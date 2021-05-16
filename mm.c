/*
 * mm.c
 *
 * Name: Stephen Galucy and Jarrett Parker
 *
 * Init: Our init function is based off of code from CS:APP Chapter 9.9.
 * The init function starts by creating a heap with the mem_sbrk function.
 * It then creates the prologue header and footer along with an epilogue header. These allow for us to know when we reach the beginning and end of our heap.
 * Lastly, the extend_heap function is called to give our heap a large free block to use.
 * 
 * Malloc: Our malloc function is based off of code from CS:APP Chapter 9.9.
 * The malloc function starts off by checking to see if there is memory that needs to be malloced and it is not just a wasted call. (Done by making sure the size > 0)
 * Next we add in the header and footer size to the size of the memory request and make sure that it is 16 byte aligned before moving on.
 * After all of this is done we are able to move on and use our first fit algorithm to search through all of the blocks to find the first one that is big enough
 * to handle the request that was sent to malloc. If no free block was found then we extend the heap and add the request there.
 * 
 * Free: Our free function is based off of code from CS:APP Chapter 9.9.
 * The given pointer in the input is used to set the block header and footer to free, and then calls coalesce to see if the blocks near the newly
 * freed block are also free. If they are then they are combined into one bigger block.
 * 
 * Realloc: We made our realloc by first understanding the code from CS:APP Chapter 9.9 completely in order to give us a jump start.
 * This helped us recognize that realloc was going to require us to use our malloc and free functions that we had implemented earlier.
 * We then began our function by getting the size of the old allocated block, creating a pointer to a new block that may need to be used,
 * and aligning the input size for the new block. The next step is to determine whether or not we actually need to re-allocate the block by checking
 * the input pointer and size. If the adjusted input size is the same as the old block's size, we do nothing. If the pointer is null, we just need to malloc. 
 * If the size is 0, we need to free the block. Next we compare the old size with the new size to see if we are shrinking the block or creating a larger block.
 * If the new size is smaller, we adjust the header and footer of the old block and free the excess data from it. We just have to return the old pointer then.
 * If the size is larger, we then use memcpy to copy the data of the block over to the new larger block.
 * Lastly we free the old block and return the pointer to the new one.
 *
 * Heap Checker: Our heap checker checks the prologue header and footer, as well as the epilogue header for correctness.
 * It also checks if the header and footer of a block contain the same information. Lastly, it checks if a block is coalesced/freed properly.
 * We then wrote a loop that checked through our free list. As it ran through the free list it checks to make sure that every block that is contained in the list 
 * is actually free and able to hold memory. It also checks to make sure that the memory addresses that the pointers that are contained in each free block point
 * to valid memory locations within the heap.
 * 
 * Quirks and Other Notes:
 * To begin the project, we used the code from CS:APP Chapter 9.9. This code gave us a basis
 * to go off of. It used implicit free list structure where on a malloc call every single
 * block was iterated through until the first free block that was big enough to hold the
 * request was found. This implementation gave us the building blocks to pass all the trace
 * files successfully and give us okay scores for CP2 and CP3. We then attempted to implement an
 * explicit free list into our solution in order to increase our throughput and utilization.
 * Unfortunately this free list continued to get errors that we were unable to fix, so the list
 * is commented out in order for the code to work properly.
 *
 */
#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <stdbool.h>
#include <stdint.h>

#include "mm.h"
#include "memlib.h"

/*
 * If you want to enable your debugging output and heap checker code,
 * uncomment the following line. Be sure not to have debugging enabled
 * in your final submission.
 */
// #define DEBUG

#ifdef DEBUG
/* When debugging is enabled, the underlying functions get called */
#define dbg_printf(...) printf(__VA_ARGS__)
#define dbg_assert(...) assert(__VA_ARGS__)
#else
/* When debugging is disabled, no code gets generated */
#define dbg_printf(...)
#define dbg_assert(...)
#endif /* DEBUG */

/* do not change the following! */
#ifdef DRIVER
/* create aliases for driver tests */
#define malloc mm_malloc
#define free mm_free
#define realloc mm_realloc
#define calloc mm_calloc
#define memset mem_memset
#define memcpy mem_memcpy
#endif /* DRIVER */

/* What is the correct alignment? */
#define ALIGNMENT 16

/* rounds up to the nearest multiple of ALIGNMENT */
static size_t align(size_t x)
{
    return ALIGNMENT * ((x+ALIGNMENT-1)/ALIGNMENT);
}

#define WSIZE 8 //Word size of 8 bytes
#define DSIZE 16 //Double word size
#define CHUNKSIZE (1<<12) //Amount to extend heap by

/*
//Struct for our double linked free list
struct DLL
{
    size_t blockSize; //The size of the block
    struct DLL *next; //Pointer to next free block
    struct DLL *prev; //Pointer to previous free block
};

//Creating free list data type from DLL
typedef struct DLL freelist;
freelist *head = NULL;

//Function to add to the free list
static void addFree(void *p) {
    //Create temporary pointer for the free list
    freelist *temp = head;
    
    //Check if the list is empty
    if (head == NULL) {
        head = p;
        head -> next = NULL;
        head -> prev = NULL;
        return;
    }
    
    //Iterate through until the end is found
    while ((temp -> next) != NULL) {
        temp = temp -> next;
    }

    //Set the next value in the free list to p
    temp -> next = p;
    (temp -> next) -> prev = temp;

    // Make sure the new block was added correctly
    //mm_checkheap(331);

    return;
}

//Function to remove from the free list
static void removeFree(void *p) {
    //Create temporary pointer for the free list
    freelist *temp = head;
    
    //Check to see if we are removing the head
    if (temp == p) {
        //We set the next block in the list to the head and remove the node from the list
        head = temp -> next;
        temp -> next = NULL;
        return; 
    } 

    //Iterate through list until block is found
    while (temp != p) {
        temp = temp -> next;
    }

    //Check if block is the tail of the list
    if ((temp -> next) == NULL) {
        //We set the next pointer to the previous block to NULL and the prev pointer to NULL
        (temp -> prev) -> next = NULL;
        temp -> prev = NULL;
        return;
    } 

    //We now do the operations for removing a block in the middle of the list
    (temp -> prev) -> next = temp -> next;
    (temp -> next) -> prev = temp -> prev;
    temp -> next = NULL;
    temp -> prev = NULL;

    // Make sure the block is no longer in the free list
    //mm_checkheap(331);

    return;
}
*/


static char *heap_listp = 0;

static void *coalesce(void *p);

// Write data to the block (put the data into the block) that is at address p
static void put(void *p, size_t val) {
    *(size_t *)(p) = val;
}

// Set the allocated bit for the block, set the metadata in the header
static size_t pack(size_t size, size_t alloc) {
    return ( size | alloc );
}

//Function to return the larger of two values (made with help from book)
static size_t max(size_t x, size_t y) {
    return ( (x>y) ? x : y );
}

// Read the word that is at address p
static size_t get(void *p) {
    return ( *(size_t *)(p) );
}

// Read the size of the block that is at address p
static size_t get_size(void *p) {
    return ( get(p) & (~0xF) );
}

// Find out whether the block is allocated ( 1 ) or free ( 0 )
static size_t get_alloc(void *p) {
    return ( get(p) & 0x1 );
}

// Find the address of the header
static void *hdrp(void* p) {
    return ( p - WSIZE );
}

// Find the address of the footer
static void *ftrp(void* p) {
    return ( p + (get_size(hdrp(p)) - DSIZE) );
}

// Find the address of the next block
static void *next_blk(void *p) {
    return ( (p + get_size(p - WSIZE)) );
}

// Find the address of the previous block
static void *prev_blk(void *p) {
    return ( (p - get_size(p - DSIZE)) );
}

// Finds the first available block that can hold a malloc call of size asize
static void *find_fit(size_t asize) {
    void *bp;
    //freelist *bp;

    /*
    //Search free list to find block that fits
    for (bp = head; get_size(hdrp(bp)) > 0; bp = bp -> next) {
        if (get_size(hdrp(bp)) >= asize) {
            return bp;
        }
    }
    */

    // Search the heap to find a free block that can hold memory of size asize
    for (bp = heap_listp; get_size(hdrp(bp)) > 0; bp = next_blk(bp)) {
        // Make sure the block is not already allocated
        // And if the memory will fit into the block
        if (!get_alloc(hdrp(bp)) && (asize <= get_size(hdrp(bp)))) {
            return bp;
        }
    }

    // No fit was found
    return NULL;
}

// Will place the requested block at the beginning of the free block,
// Will split only if the size of the remmainder would equal or exceed the minimum block size
static void place(void *bp, size_t asize) {
    size_t csize = get_size(hdrp(bp));

    // If there is enough internal fragmentation then split the memory that it does not need
    // Then create a new free block with the memory that was not being used and was split off
    // From the memory that is being used
    if ((csize - asize) >= (2*DSIZE)) {
        put(hdrp(bp), pack(asize, 1));
        put(ftrp(bp), pack(asize, 1));
        bp = next_blk(bp);
        put(hdrp(bp), pack(csize-asize, 0));
        put(ftrp(bp), pack(csize-asize, 0));
        coalesce(bp);

        // Make sure the epilogue is still correct
        //mm_checkheap(169);
    }
    else { // Don't worry about splitting and just put the memory in the block
        put(hdrp(bp), pack(csize, 1));
        put(ftrp(bp), pack(csize, 1));
    }
}

// Extend the heap with a new free block of size EXTENDSIZE bytes
static void *extend_heap(size_t words) {
    char *bp;
    size_t size;

    /* Make sure there are an even number of words in order to maintain alignment
     * and allocate those words
     */
    //      if this       then do this        else do this
    size = (words % 2) ? (words+1) * WSIZE : words * WSIZE;
    if ((long)(bp = mem_sbrk(size)) == -1) {
        return NULL;
    }

    /* Initialize the free block header and footer
     * and the new epilogue header
     */
    put(hdrp(bp), pack(size, 0)); // Create the free block header and set metadata values
    put(ftrp(bp), pack(size, 0)); // Create the free block footer and set metadata values
    put(hdrp(next_blk(bp)), pack(0, 1)); // Create a new epilogue header and set metadata values

    // Make sure that the new epilogue is set correctly
    //mm_checkheap(199);

    // Coalesce free blocks if the previous block was free
    return coalesce(bp);
}

/* Check blocks before and after block pointed to by p
 * If either is free, then combine them into 1 bigger block
 */
static void *coalesce(void *bp) {
    size_t prev_alloc = get_alloc(ftrp(prev_blk(bp)));
    size_t next_alloc = get_alloc(hdrp(next_blk(bp)));
    size_t size = get_size(hdrp(bp));

    /* Case 1
     * If both adjacent blocks are allocated then do nothing and return the pointer
     */
    if (prev_alloc && next_alloc) {
        //addFree(bp); 
        return bp;
    }

    /* Case 2
     * If the next block is free then, update the metadata in both
     * the header and footer and set the alocated bit to 0 (free)
     */
    else if (prev_alloc && !next_alloc) { 
        size += get_size(hdrp(next_blk(bp)));
        //removeFree(next_blk(bp));
        put(hdrp(bp), pack(size, 0));
        put(ftrp(bp), pack(size,0));
        //addFree(bp);
    }

    /* Case 3
     * If the previous block is free then, update the metadata in both
     * the header and footer and set the allocated bit to 0 (free) then,
     * change the pointer to the new location for the free block
     */
    else if (!prev_alloc && next_alloc) { 
        size += get_size(hdrp(prev_blk(bp)));
        //removeFree(prev_blk(bp));
        put(ftrp(bp), pack(size, 0));
        put(hdrp(prev_blk(bp)), pack(size, 0));
        bp = prev_blk(bp);
        //addFree(bp);
    }

    /* Case 4
     * If both the previous and next block are free, then update the metadata in both
     * the header and footer and set the allocated bit to 0 (free) then,
     * change the pointer to the new location for the free block
     */
    else { 
        size += get_size(hdrp(prev_blk(bp))) + get_size(ftrp(next_blk(bp)));
        //removeFree(prev_blk(bp));
        //removeFree(next_blk(bp));
        put(hdrp(prev_blk(bp)), pack(size, 0));
        put(ftrp(next_blk(bp)), pack(size, 0));
        bp = prev_blk(bp);
        //addFree(bp);
    }

    // Make sure that the blocks are coalesced properly
    //mm_checkheap(252);

    return bp;
}


/*
 * Initialize: returns false on error, true on success.
 */
bool mm_init(void)
{
    /* IMPLEMENT THIS */

    /* Create the Initial Heap which will be empty,
     * Including the prologue header and footer
     * and epilogue footer. These are in order to easily tell if you reach the end
     * of the heap or if you cannot coalesce any farther to the right at the
     * beginning of the heap.
     */
    if ((heap_listp = mem_sbrk(4*WSIZE)) == (void *)-1) {
        return (false);
    }
    put(heap_listp, 0); // Pad for the alignment
    put(heap_listp + (1*WSIZE), pack(DSIZE, 1)); //Create prologue header
    put(heap_listp + (2*WSIZE), pack(DSIZE, 1)); //Create prologue footer
    put(heap_listp + (3*WSIZE), pack(0, 1)); //Create epilogue header
    heap_listp += (2*WSIZE); // Set the heap pointer past the prologue and pointing to the first free block

    // Check that the prologue and epilogue are correct
    //mm_checkheap(275);

    /* Extend the empty heap with a free block of CHUNKSIZE bytes */
    if (extend_heap(CHUNKSIZE/WSIZE) == NULL) {
        return (false);
    }

    return (true);
}


/*
 * malloc
 */
void* malloc(size_t size)
{
    /* IMPLEMENT THIS */

    /*
     * Align the amount of memory space requested to the required 16 bytes.
     * Search through the list to find a free block that can accomandate the amount of memory given by size.
     * If no block big enough is found, then add more memory to the heap and add it.
     */
    size_t asize; // Adjusted block size
    size_t extendsize; // Amount to extend heap if no block is big enough
    char *bp;

    // Ignore if there is nothing that needs to be malloced
    if (size == 0) {
        return NULL;
    }

    // Change the block size to include the header/footer and 16 bit alignment
    if (size <= DSIZE) {
        asize = 2*DSIZE;
    }
    else {
        asize = DSIZE * ((size + (DSIZE) + (DSIZE-1)) / DSIZE);
    }

    // Search for a block that fits that is in the free list
    if ((bp = find_fit(asize)) != NULL) {
        place(bp, asize);
        return bp;
    }

    // No free block big enough was found
    // Extend the heap and assign the memory to a block
    extendsize = max(asize,CHUNKSIZE);
    if ((bp = extend_heap(extendsize/WSIZE)) == NULL) {
        return NULL;
    }

    place(bp, asize);

    // Make sure that the header and footer have the same information
    //mm_checkheap(331);

    return bp;
}

/*
 * free
 */
void free(void* ptr)
{
    /* IMPLEMENT THIS */

    /* Free the memory that is located at the given pointer
     * Change the metadata to say that the block is free and is available to be allocated
     * Coalesce if a block near it is also free
     */
    size_t size = get_size(hdrp(ptr));
    put(hdrp(ptr), pack(size, 0));
    put(ftrp(ptr), pack(size, 0));
    coalesce(ptr);
}

/*
 * realloc
 */
void* realloc(void* oldptr, size_t size)
{
    /* IMPLEMENT THIS */

    // Copy old data into newly malloc'ed block
    size_t old_size = get_size(hdrp(oldptr));
    // Pointer to the newly allocated block
    void *new_p = malloc(size);
    // Adjust the size to fit alignment
    size_t adjust_size = max(align(size) + DSIZE, 32);

    // Check if adjusted size is the same as the old size
    // If they are then no realloc is necessary
    if (adjust_size == old_size) {
        return ( oldptr );
    }

    // Check if oldptr is NULL or size is 0
    if (oldptr == NULL) {
        // We need to malloc
        return (malloc(size));
    }

    // If size is 0, we are freeing
    if (size == 0) {
        // We need to free
        free(oldptr);
        return ( 0 );
    }

    // Check if adjusted size is less than the old size
    // If it is then we need to realloc that memory to a new block
    // And then free the old block to be reused
    if (adjust_size <= old_size) {
        put(hdrp(oldptr), pack(adjust_size, 1));
        put(ftrp(oldptr), pack(adjust_size, 1));
        put(hdrp(next_blk(oldptr)), pack(old_size - adjust_size, 1));
        put(ftrp(next_blk(oldptr)), pack(old_size - adjust_size, 1));
        free(next_blk(oldptr));
        return ( oldptr );
    }

    // Check to see if malloc was properly executed
    if (new_p == NULL) {
        return ( 0 );
    }

    // Copy old data into newly malloc'ed block
    // Compare the sizes of the new and old allocations
    if (size < old_size) {
        old_size = size;
    }

    memcpy(new_p, oldptr, old_size);
    free(oldptr); // Free the old block

    return ( new_p );
}

/*
 * calloc
 * This function is not tested by mdriver, and has been implemented for you.
 */
void* calloc(size_t nmemb, size_t size)
{
    void* ptr;
    size *= nmemb;
    ptr = malloc(size);
    if (ptr) {
        memset(ptr, 0, size);
    }
    return ptr;
}

/*
 * Returns whether the pointer is in the heap.
 * May be useful for debugging.
 */
static bool in_heap(const void* p)
{
    return p <= mem_heap_hi() && p >= mem_heap_lo();
}

/*
 * Returns whether the pointer is aligned.
 * May be useful for debugging.
 */
static bool aligned(const void* p)
{
    size_t ip = (size_t) p;
    return align(ip) == ip;
}

/*
 * mm_checkheap
 */
bool mm_checkheap(int lineno)
{
#ifdef DEBUG
    /* Write code to check heap invariants here */
    /* IMPLEMENT THIS */

    /* Check for consistencies, inconsistencies and invariants in the heap
     * Return true if there is nothing found to be wrong
     * Return false if there is an error that is found
     */

    char *p = heap_listp;
    //size_t prev_alloc, curr_alloc;
    //freelist *fp = head;

    // Checks to make sure that the prologue header is correctly initialized and at the beginning of the heap
    // Checked mainly in the mm_init() call to this function and then again when a block is split in place()
    if ( (get_size(hdrp(heap_listp)) != DWORD || !get_alloc(hdrp(heap_listp))) ) {
        printf("Prologue Header Check: Not Initialized Properly: Line %d", lineno);
        return false;
    }

    // Checks to make sure that the epilogue header is correctly initialized and at the end of the heap
    // Checked mainly in the mm_init() call to this function and then again when a block is split in place()
    if ( (get_size(hdrp(p)) != 0 || !(get_alloc(hdrp(p)))) ) {
        printf("Epilogue Header Check: Not Initialized Properly: Line %d", lineno);
        return false;
    }

    // Check to make sure the metadata in the header and footer of a given block contain the same information
    if ( get(hdrp(heap_listp)) != get(ftrp(heap_listp)) ) {
        printf("Header does Not Match Footer: Line %d", lineno);
        return false;
    }

    // Checks for adjacent free block that was never coalesced
    if ( !get_alloc(hdrp(p)) ) {
        // Check the next block to see if it is also free
        if ( !get_alloc(hdrp(next_blk(p))) ) {
            printf("Adjacent Free Block Found: Next Block: Line %d", lineno);
            return false;
        }
        // Check the previous block to see if it is also free
        else if ( !get_alloc(ftrp(prev_blk(p))) ) {
            printf("Adjacent Free Block Found: Previous Block: line %d", lineno);
            return false;
        }
    }

    /*
        //Check if the free list has all free blocks
        for (fp; fp != NULL; fp = fp -> next) {
            // Check to make sure that every block in the list is atually free
            if (get_alloc(fp) != 0) {
                printf("Block in free list is not actually free: line %d", lineno);
                return false;
            }
            //Check if the free block lies within the heap
            if ((fp < mem_heap_lo) || (fp > mem_heap_hi)) {
                printf("Block in the free list is not in the heap: line %d", lineno);
                return false;
            }
        }
        */

    //return false:

#endif /* DEBUG */
    return true;
}
