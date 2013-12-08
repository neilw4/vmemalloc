#include <stdint.h>
#include <unistd.h>
#include <sys/mman.h>

#include "vmemalloc.h"
#include "vmemalloc_large.h"

// Linked lists of free chunks with sizes in the same order of magnitude.
FreeChunkHeader* bins[NUM_BINS]; // {1, 2, 3-4, 5-8...}

// Index of the largest bin with a free chunk. -1 if there are no free chunks.
int largestBinInUse = -1;

// Converts a free chunk into an allocated chunk.
ChunkHeader* makeChunkAllocated(FreeChunkHeader* chunk) {
    SET_CHUNK_FREE(chunk, false);
    if (!GET_LAST_CHUNK_OF_REGION(chunk)) {
        ChunkHeader* nextChunk = (ChunkHeader*)((void*)chunk + GET_SIZE(chunk));
        SET_PREVIOUS_CHUNK_FREE(nextChunk, false);
    }
    return (ChunkHeader*) chunk;
}

// Converts an allocated chunk into a free chunk.
FreeChunkHeader* makeChunkFree(ChunkHeader* chunk) {
    SET_CHUNK_FREE(chunk, true);
    if (!GET_LAST_CHUNK_OF_REGION(chunk)) {
        ChunkHeader* nextChunk = (ChunkHeader*)((void*)chunk + sizeof(ChunkHeader) + GET_SIZE(chunk));
        SET_PREVIOUS_CHUNK_FREE(nextChunk, true);
    }
    CREATE_FREE_CHUNK_FOOTER(chunk);
    return (FreeChunkHeader*) chunk;
}

// Creates a new allocated chunk at the given location.
void initAllocdChunk(ChunkHeader* chunk, Word size, Word lastChunkOfRegion, Word previousChunkFree) {
    RESET_HEADER(chunk);
    SET_LAST_CHUNK_OF_REGION(chunk, lastChunkOfRegion);
    SET_PREVIOUS_CHUNK_FREE(chunk, previousChunkFree);
    SET_SIZE(chunk, size);
    makeChunkAllocated((FreeChunkHeader*)chunk);
}

// Creates a new free chunk at the given location.
void initFreeChunk(FreeChunkHeader* chunk, Word size, Word lastChunkOfRegion, Word previousChunkFree) {
    RESET_HEADER(chunk);
    SET_LAST_CHUNK_OF_REGION(chunk, lastChunkOfRegion);
    SET_PREVIOUS_CHUNK_FREE(chunk, previousChunkFree);
    SET_SIZE(chunk, size);
    makeChunkFree((ChunkHeader*)chunk);
}

// Gets the bin index for the corresponding size.
int getBin(int size) {
    int sizeInWords = size / sizeof(Word);
    // Special case to avoid log(0).
    if (sizeInWords == 1) {
        return 0;
    }
    return (int)log2(sizeInWords - 1) + 1;
}

// Add a free chunk to the correct bin.
void addChunkToBin(FreeChunkHeader* chunk) {
    int chunkSize = (int)GET_SIZE(chunk);
    int bin = getBin(chunkSize);
    // Add to the linked list.
    chunk->nextFree = bins[bin];
    if (chunk->nextFree != NULL) {
        chunk->nextFree->lastFree = chunk;
    }
    bins[bin] = chunk;
    chunk->lastFree = NULL;
    // Update largest bin in use.
    if (bin > largestBinInUse) {
        largestBinInUse = bin;
    }
    freeSpace += chunkSize;
    freeChunkCount++;
}

// Remove a free chunk from its bin.
void removeChunkFromBin(FreeChunkHeader* chunk) {
    int chunkSize = (int)GET_SIZE(chunk);
    int bin = getBin(chunkSize);

    // Update linked list.
    if (bins[bin] == chunk) {
        bins[bin] = chunk->nextFree;
    }
    if (chunk->nextFree != NULL) {
        chunk->nextFree->lastFree = chunk->lastFree;
    }
    if (chunk->lastFree != NULL) {
        chunk->lastFree->nextFree = chunk->nextFree;
    }
    chunk->nextFree = NULL;
    chunk->lastFree = NULL;

    // If it was removed from the largest bin, there may be a new largest bin.
    if (bin == largestBinInUse) {
        // Find the largest non-empty bin.
        while (largestBinInUse >= 0 && bins[largestBinInUse] == NULL) {
            largestBinInUse--;
        }
    }
    freeSpace -= chunkSize;
    freeChunkCount--;
}

// Use mmap to create a new region containing an allocated chunk.
ChunkHeader* newRegion(int size) {
    // Regions created by mmap are always a multiple of the page size.
    Word regionSize = CEIL(size + sizeof(FreeChunkHeader*), getpagesize());
    // For some reason, mapping without PROT_EXEC creates regions that are larger than requested.
    ChunkHeader* region = (ChunkHeader*) mmap(0, (int)regionSize, PROT_READ|PROT_WRITE|PROT_EXEC, MAP_PRIVATE|MAP_ANONYMOUS, -1, 0);
    if (region == NULL) {
        perror("Error creating new region");
        return NULL;
    }
    // Offset start of chunk so that chunk (after header) is aligned to LARGEST_ALIGNMENT.
    ChunkHeader* chunk = (ChunkHeader*)((void*)region + ALIGNMENT_OFFSET);
    int chunkSize = regionSize - sizeof(RegionFooter) - sizeof(ChunkHeader) - ALIGNMENT_OFFSET;
    initAllocdChunk(chunk, chunkSize, true, false);
    // Region footer points to chunk at start of region.
    CREATE_REGION_FOOTER(region, regionSize, chunk);
    regionsUsed++;
    return chunk;
}

// Use munmap to remove a region previously created by mmap.
void removeRegion(FreeChunkHeader* chunk) {
    makeChunkAllocated(chunk);
    void* region = (void*)chunk - ALIGNMENT_OFFSET;
    int regionSize = ALIGNMENT_OFFSET + sizeof(ChunkHeader) + GET_SIZE(chunk) + sizeof(RegionFooter);
    if (munmap(region, regionSize)) {
        perror("Error in munmap");
    }
    regionsUsed--;
}

// Returns true if the chunk fills a whole region created by mmap.
Word chunkFillsRegion(FreeChunkHeader* chunk) {
    // Only the last chunk can possibly fill the region.
    if (!GET_LAST_CHUNK_OF_REGION(chunk)) {
        return false;
    }
    RegionFooter* footer = GET_REGION_FOOTER(chunk);
    return *footer == (ChunkHeader*)chunk;
}

// Finds (or creates) an chunk where GET_SIZE(chunk) >= size.
// Chunk is removed from bins and allocated.
ChunkHeader* findFreeChunk(Word minSize) {
    int bestFitBin = getBin(minSize);
    if (largestBinInUse >= bestFitBin) {
        // First search the bin with the right sized chunks (best-fit).
        FreeChunkHeader* chunk = bins[bestFitBin];
        while (chunk != NULL) {
            Word chunkSize = GET_SIZE(chunk);
            // Best-fit is only efficient if the block will not be split.
            if (chunkSize >= minSize && chunkSize < minSize + MIN_CHUNK_SIZE) {
                removeChunkFromBin(chunk);
                return makeChunkAllocated(chunk);
            }
            chunk = chunk->nextFree;
        }

        if (largestBinInUse > bestFitBin) {
            // Take a chunk from the largest bin (worst-fit).
            chunk = bins[largestBinInUse];
            removeChunkFromBin(chunk);
            return makeChunkAllocated(chunk);
        } else {
            // Search the bin with the right sized chunks (best-fit), but less fussily.
            chunk = bins[bestFitBin];
            while (chunk != NULL) {
                Word chunkSize = GET_SIZE(chunk);
                // Consider blocks that would be split this time.
                if (chunkSize >= minSize) {
                    removeChunkFromBin(chunk);
                    return makeChunkAllocated(chunk);
                }
                chunk = chunk->nextFree;
            }
        }
    }
    // If there are no suitable chunks, create a new region.
    return newRegion(minSize);
}

// Returns a suitable chunk for use by a program.
void* vmemallocLarge(int sizeRequested) {
    Word size = (Word)sizeRequested;

    // Enforce minimum size.
    if (size < MIN_CHUNK_SIZE) {
        size  = MIN_CHUNK_SIZE;
    }
    // Minimum size s.t. size >= sizeRequested and n is an integer where
    // size = LARGEST_ALIGNMENT * n + ALIGNMENT_OFFSET
    // This is required to ensure that chunks (not headers) are aligned.
    size = CEIL(size - ALIGNMENT_OFFSET, LARGEST_ALIGNMENT) + ALIGNMENT_OFFSET;

    // Highest bit of the chunk size must be zero to allow chunk headers to function.
    if (GET_CHUNK_FREE(&size)) {
        fprintf(stderr, "Too big to allocate\n");
        return NULL;
    }

    ChunkHeader* chunk = findFreeChunk(size);
    if (chunk == NULL) {
        fprintf(stderr, "Free chunk returned by findFreeChunk to vmemallocLarge was NULL\n");
        return NULL;
    }
    Word chunkSize = GET_SIZE(chunk);
    if (chunkSize >= (size + MIN_CHUNK_SIZE)) {
        // Split it up into two chunks, where the second chunk is free.
        FreeChunkHeader* nextChunk = (FreeChunkHeader*) ((void*)chunk + sizeof(ChunkHeader) + size);
        Word nextChunkSize = chunkSize - size - sizeof(ChunkHeader);
        initFreeChunk(nextChunk, nextChunkSize, GET_LAST_CHUNK_OF_REGION(chunk), false);
        addChunkToBin(nextChunk);

        SET_LAST_CHUNK_OF_REGION(chunk, false);
        SET_SIZE(chunk, size);
        chunkSize = size;
    }
    allocatedSpace += chunkSize;
    // Use the free memory after the header.
    return (void*)chunk + sizeof(ChunkHeader);
}

// Frees a chunk used by a program so it can be reused.
// Returns the amount of space saved.
int vmemfreeLarge(void* ptr) {
    // Find the header.
    ChunkHeader* allocdChunk = ptr - sizeof(ChunkHeader);
    if(GET_CHUNK_FREE(allocdChunk)) {
        fprintf(stderr, "Tried to free a free block\n");
        return -1;
    }
    FreeChunkHeader* chunk = makeChunkFree(allocdChunk);
    int spaceSaved = (int)GET_SIZE(chunk);

    if (! GET_LAST_CHUNK_OF_REGION(chunk)) {
        // Coalesce forwards if possible.
        ChunkHeader* nextChunk = (ChunkHeader*)((void*)chunk + sizeof(chunk) + GET_SIZE(chunk));
        if (GET_CHUNK_FREE(nextChunk)) {
            removeChunkFromBin((FreeChunkHeader*)nextChunk);
            Word newSize = GET_SIZE(chunk) + sizeof(ChunkHeader) + GET_SIZE(nextChunk);
            SET_SIZE(chunk, newSize);
            SET_LAST_CHUNK_OF_REGION(chunk, GET_LAST_CHUNK_OF_REGION(nextChunk));
        }
    }
    if (GET_PREVIOUS_CHUNK_FREE(chunk)) {
        // Coalesce backwards.
        // Previous footer should be just before the header, and point to the previous chunk.
        FreeChunkFooter* previousChunkFooter = (FreeChunkFooter*)((void*)chunk - sizeof(FreeChunkFooter));
        FreeChunkHeader* previousChunk = *previousChunkFooter;
        removeChunkFromBin(previousChunk);
        Word newSize = GET_SIZE(previousChunk) + sizeof(ChunkHeader) + GET_SIZE(chunk);
        SET_SIZE(previousChunk, newSize);
        SET_LAST_CHUNK_OF_REGION(previousChunk, GET_LAST_CHUNK_OF_REGION(chunk));
        chunk = previousChunk;
    }
    // Make a footer for the chunk.
    CREATE_FREE_CHUNK_FOOTER(chunk);

    if (chunkFillsRegion(chunk)) {
        // Unmap the region.
        removeRegion(chunk);
    } else {
        // Add to the bins for reuse.
        addChunkToBin(chunk);
    }
    allocatedSpace -= spaceSaved;
    return spaceSaved;
}
