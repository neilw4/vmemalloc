#ifndef VMEMALLOC_SMALL_GUARD
#define VMEMALLOC_SMALL_GUARD

// Semi arbitrary - need to balance container size so it's not too small or too large.
#define CHUNKS_PER_CONTAINER ((CHAR_BIT * sizeof(Word)) / 4)

// Container for lots of small chunks. Implemented as alloc'd chunk.
typedef union ContainerHeader {
    struct {
        // part of a linked list of containers.
        union ContainerHeader* nextContainer;
        // Bitmap where the nth bit represents the freeness of the nth chunk.
        Word mask;
    } s;
    LARGEST_ALIGNMENT_TYPE padding;
} ContainerHeader;

// Returns a suitable chunk for use by a program.
// Fast but inefficient implementation for small chunks.
extern void* vmemallocSmall(int size);

// Frees a chunk used by a program so it can be re-used.
// Returns the amount of space saved or 0 if ptr wasn't created by vmemallocSmall.
// Fast but inefficient implementation for small chunks.
extern int vmemfreeSmall(void* ptr);

#endif
