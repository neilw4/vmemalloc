#ifndef VMEMALLOC_LARGE_GUARD
#define VMEMALLOC_LARGE_GUARD

// A chunk header is stored in one word and contains lots of data, accessible by bitmasks.

// Header is stored in a word, and uses the form
typedef Word ChunkHeader;

// Free chunks are part of a doubly linked list of free chunks.
typedef struct FreeChunkHeader {
    ChunkHeader header;
    struct FreeChunkHeader* lastFree;
    struct FreeChunkHeader* nextFree;
} FreeChunkHeader;

// Free chunk footer points to the start of the chunk.
typedef FreeChunkHeader* FreeChunkFooter;

// Region footer points to the start of the region.
typedef ChunkHeader* RegionFooter;

// Because it is used in bitwise masks, true must be ~0, as the bitmask may exclude some bits.
#define true ((Word)~0)
#define false ((Word)0)

// Assuming 32 bit (or higher) word size, the lowest two bits of the header can be reused
// because the chunk size must be a multiple of the word size.
// The first bit can be used because we assume that no chunk will be larger than half of the address space.

// Lowest bit indicates whether it is the last chunk in the malloced region.
#define LAST_CHUNK_OF_REGION_MASK 0x1 // 0x0...01
// Second lowest bit indicates whether the previous chunk is free.
#define PREVIOUS_CHUNK_FREE_MASK 0x2 // 0x0...010
// Highest bit indicates whether the chunk is free.
#define CHUNK_FREE_MASK ~(~(Word)0 >> 1) // 0x10...0
// The other bits are used to define the size of the chunk.
#define SIZE_MASK ~(LAST_CHUNK_OF_REGION_MASK | PREVIOUS_CHUNK_FREE_MASK | CHUNK_FREE_MASK)


#define RESET_HEADER(chunk) *(ChunkHeader*)chunk = (ChunkHeader)0
// Applies a bitmask to get the right data from the header.
#define GET_FROM_HEADER(chunk, mask) ((chunk) & (mask))
// Note value is bitwise, so true must be represented as ~0 instead of 1.
#define MODIFY_HEADER(chunk, mask, value) chunk = (((ChunkHeader)(chunk)) & ~(mask)) | ((mask) & (value))

// Getters for the header.
#define GET_LAST_CHUNK_OF_REGION(chunkPtr) GET_FROM_HEADER(*(ChunkHeader*)chunkPtr, LAST_CHUNK_OF_REGION_MASK)
#define GET_PREVIOUS_CHUNK_FREE(chunkPtr) GET_FROM_HEADER(*(ChunkHeader*)chunkPtr, PREVIOUS_CHUNK_FREE_MASK)
#define GET_CHUNK_FREE(chunkPtr) GET_FROM_HEADER(*(ChunkHeader*)chunkPtr, CHUNK_FREE_MASK)
#define GET_SIZE(chunkPtr) GET_FROM_HEADER(*(ChunkHeader*)chunkPtr, SIZE_MASK)

// Setters for the header.
#define SET_LAST_CHUNK_OF_REGION(chunkPtr, value) MODIFY_HEADER(*(ChunkHeader*)chunkPtr, LAST_CHUNK_OF_REGION_MASK, (Word)value)
#define SET_PREVIOUS_CHUNK_FREE(chunkPtr, value) MODIFY_HEADER(*(ChunkHeader*)chunkPtr, PREVIOUS_CHUNK_FREE_MASK, (Word)value)
#define SET_CHUNK_FREE(chunkPtr, value) MODIFY_HEADER(*(ChunkHeader*)chunkPtr, CHUNK_FREE_MASK, (Word)value)
#define SET_SIZE(chunkPtr, value) MODIFY_HEADER(*(ChunkHeader*)chunkPtr, SIZE_MASK, (Word)value)

// Footer is at end of free chunk or region.
#define GET_FREE_CHUNK_FOOTER(chunk) (FreeChunkFooter*)((void*)chunk + sizeof(ChunkHeader) + GET_SIZE(chunk) - sizeof(FreeChunkFooter))
#define GET_REGION_FOOTER(chunk) (RegionFooter*)((void*)chunk + sizeof(ChunkHeader) + GET_SIZE(chunk))

// Footer always points to the start of the chunk or region.
#define CREATE_FREE_CHUNK_FOOTER(chunk) *GET_FREE_CHUNK_FOOTER(chunk) = (FreeChunkFooter)chunk
#define CREATE_REGION_FOOTER(region, size, firstChunk) *(RegionFooter*)((void*)region + size - sizeof(RegionFooter)) = (RegionFooter)firstChunk

// Number of bins to store chunks with sizes in the same order of magnitude.
#define NUM_BINS (sizeof(Word) * CHAR_BIT)

// A chunk must be able to hold all data stored in a free chunk.
#define MIN_CHUNK_SIZE (sizeof(FreeChunkHeader) - sizeof(ChunkHeader) + sizeof(FreeChunkFooter))

// Returns a suitable chunk for use by a program.
extern void* vmemallocLarge(int size);

// Frees a chunk used by a program so it can be re-used.
// Returns the amount of space saved.
extern int vmemfreeLarge(void* ptr);

#endif