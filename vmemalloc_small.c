#include "vmemalloc.h"
#include "vmemalloc_large.h"
#include "vmemalloc_small.h"

// Small chunks are put in containers with the appropriate size (sizes are powers of 2)
ContainerHeader* smallBins[SMALL_CHUNK_LIMIT_POWER] = {NULL}; // {1, 2, 3-4, 5-8...}

// Finds the bin for the corresponding size.
int getSmallBin(int size) {
    // Special case to avoid log(0).
    if (size == 1) {
        return 0;
    }
    return (int)log2(size - 1) + 1;
}

// The maximum size of a chunk in the bin.
int getSmallBinChunkSize(int bin) {
    // 2^bin
    return (1 << bin);
}

// Creates a new container and adds it to the correct bin.
void addNewContainer(int bin, int chunkSize) {
    int sizeToAlloc = sizeof(ContainerHeader) + (CHUNKS_PER_CONTAINER * chunkSize);
    ContainerHeader* container = (ContainerHeader*) vmemallocLarge(sizeToAlloc);
    // Space allocated by vmemallocLarge isn't really allocated.
    allocatedSpace -= sizeToAlloc;
    freeSpace += CHUNKS_PER_CONTAINER * chunkSize;
    if (container == NULL) {
        fprintf(stderr, "new mallocd container was null\n");
    }
    container->s.mask = (Word)0;
    container->s.nextContainer = smallBins[bin];
    smallBins[bin] = container;
}

// Returns a suitable chunk for use by a program.
// Fast but inefficient implementation for small chunks.
void* vmemallocSmall(int size) {
    int bin = getSmallBin(size);
    int chunkSize = getSmallBinChunkSize(bin);
    // If there are no suitably sized containers.
    if (smallBins[bin] == NULL) {
        addNewContainer(bin, chunkSize);
    }

    ContainerHeader* container = smallBins[bin];
    // Find a bit in the mask signifying a free space.
    Word mask = (Word)1;
    for (int i = 0; (Word)i < CHUNKS_PER_CONTAINER; i++) {
        if (!(container->s.mask & mask)) {
            // Found a free positiion.
            // Flip the bit for the position.
            container->s.mask |= mask;
            allocatedSpace += chunkSize;
            freeSpace -= chunkSize;
            // Calculate position of chunk to return.
            return (void*)container + sizeof(ContainerHeader) + (i * chunkSize);
        }
        // Weird Haskell. Or possibly bit shifting the mask to the next position.
        mask <<= 1;
    }

    // If we get to here, the container must be full, so make a new one.
    addNewContainer(bin, chunkSize);
    // New container will always be at the top of the list.
    container = smallBins[bin];
    // Use the first chunk.
    container->s.mask = (Word)1;
    allocatedSpace += chunkSize;
    freeSpace -= chunkSize;
    return (void*)container + sizeof(ContainerHeader);
}

// Frees a chunk used by a program so it can be re-used.
// Returns the amount of space saved or 0 if ptr wasn't created by vmemallocSmall.
// Fast but inefficient implementation for small chunks.
int vmemfreeSmall(void* ptr) {
    // Search bins for the right container.
    for (int bin = 0; bin < SMALL_CHUNK_LIMIT_POWER; bin++) {
        int chunkSize = getSmallBinChunkSize(bin);
        int containerSize = sizeof(ContainerHeader) + (CHUNKS_PER_CONTAINER * chunkSize);
        // May need to modify pointer to container if removing container from the list.
        ContainerHeader** containerPtr = &smallBins[bin];
        ContainerHeader* container = smallBins[bin];
        // Search all containers in the bin.
        while (container != NULL) {
            // If pointer is inside the container.
            if (ptr >= (void*)container + sizeof(ContainerHeader) && ptr < (void*)container + containerSize) {
                int ptrIndex = (ptr - sizeof(ContainerHeader) - (void*)container) / chunkSize;
                // Mark the chunk as free.
                container->s.mask &= ~(((Word)1) << ptrIndex);
                allocatedSpace -= chunkSize;
                freeSpace += chunkSize;
                if (container->s.mask == (Word)0) {
                    // Container is completely empty and can be removed.
                    *containerPtr = container->s.nextContainer;
                    vmemfreeLarge(container);
                    allocatedSpace += sizeof(ContainerHeader) + (CHUNKS_PER_CONTAINER * chunkSize);
                    freeSpace -= CHUNKS_PER_CONTAINER * chunkSize;
                }
                return chunkSize;
            }
            containerPtr = &container->s.nextContainer;
            container = container->s.nextContainer;
        }
    }
    return 0;
}