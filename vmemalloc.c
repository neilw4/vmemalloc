#include "vmemalloc.h"
#include "vmemalloc_large.h"
#include "vmemalloc_small.h"

#define VMEMALLOC_OP "vmemalloc"
#define VMEMFREE_OP "vmemfree"

/*  Allocate 'size' bytes of memory. On success the function returns a pointer to 
    the start of the allocated region. On failure NULL is returned. */
void* vmemalloc(int size) {
    if (size <= 0) {
        fprintf(stderr, "size passed to vmemalloc was too small (%d)\n", size);
        return NULL;
    }
    void* chunk;
    // Treat small chunks differently.
    if (size < SMALL_CHUNK_LIMIT) {
        chunk = vmemallocSmall(size);
        if (chunk == NULL) {
            fprintf(stderr, "error in vmemallocSmall(%i)\n", size);
            return NULL;
        }
    } else {
        chunk = vmemallocLarge(size);
        if (chunk == NULL) {
            fprintf(stderr, "error in vmemallocLarge(%d)\n", size);
            return NULL;
        }
    }
    allocatedChunkCount++;
    outputTraceData(VMEMALLOC_OP);
    return chunk;
}

/*  Release the region of memory pointed to by 'ptr'. */
void vmemfree(void* ptr) {
    if (ptr == NULL) {
        fprintf(stderr, "pointer passed to vmemfree was NULL\n");
        return;
    }
    int spaceFreed = vmemfreeSmall(ptr);
    if (spaceFreed < 0) {
        fprintf(stderr, "error in vmemfreeSmall(%p)\n", ptr);
        return;
    }
    // vmemfreeSmall returns 0 if the chunk is not a small chunk.
    if (spaceFreed == 0) {
        spaceFreed = vmemfreeLarge(ptr);
        if (spaceFreed <= 0) {
            fprintf(stderr, "error in vmemfreeLarge(%p)\n", ptr);
            return;
        }
    }
    allocatedChunkCount--;
    outputTraceData(VMEMFREE_OP);
}
