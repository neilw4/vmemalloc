#Virtual memory allocation library (malloc/free)
The program takes two different approaches to memory allocation. Small chunks are allocated in a way that is fast and aims to reduce memory overhead. Larger chunks are allocated by a cleverer algorithm that aims to reduce fragmentation.

 The program aims to be fast by minimising list traversals where possible and, where impossible, reducing the size of the lists (e.g. splitting lists into bins). Data structures and returned chunks are aligned to multiple of sizeof(long double) or, if they are smaller, aligned to multiples of their own size.
 
vmemalloc.c determines whether to treat chunks as small or large chunks. These are dealt with by vmemalloc_small.c and vmemalloc_large.c. logger.c deals with storing and outputting statistics about the memory usage. The makefile will build version of the vmemalloc library with and without this logging information.
#Large Chunk Organisation
##Allocated Chunks
Chunks have a header immediately before the useable chunk of memory.  Chunk sizes are always multiples of sizeof(long double) to make sure that they are properly aligned. The chunk header contains the size of the chunk and flags indicating whether it is the last chunk in a region created by mmap, whether the previous chunk is free, and whether the chunk itself is free. The header takes up a single word.

We can assume that the highest bit and the two lowest bits are always zero. and can be used for flags. If the word size is 32 or 64 bits, then the chunk size must be a multiple of 4 bytes, therefore the lowest two bits will always be zero. We can also assume that all allocated chunks will be less than half of the size of the address space (i.e. for a 32 bit system, chunks will be less than 2GB. These assumptions allow us to pack all of the data we need into a single word.
##Free Chunks
In addition to a header, a free chunk also contains a footer and a doubly linked list. The footer is at the very end of the chunk, and contains a pointer to the start of the chunk. The first two words of the free chunk are pointers to other free chunks, as part of a doubly linked list.
##Bins
In order to quickly access free chunks with the right size, I group free chunks into bins of similarly sized chunks (stored in an array). The nth bin points to a doubly linked list of all free chunks with a size between 2<sup>n-1</sup>+1 and 2<sup>n</sup> words. 
##Regions
A block of memory created by anonymous mmap is referred to as a region. Regions have a footer which points to the first chunk in the region.
##Allocation Algorithm
To prevent fragmentation, the allocation algorithm should aim for a high average free chunk size. It uses the bins to do this efficiently. Firstly, the bin with the right sized chunks are searched to find a suitable chunk to use (best-fit). If no suitable chunks are found, it uses a block from the largest bin (worst-fit). If there are no chunks of a suitable size, it uses mmap to get more memory pages.
##Freeing Algorithm
To free a chunk of memory, the program finds the header using using the algorithm ptr-sizeof(ChunkHeader). If possible, the chunk is coalesced with the preceding and succeeding free chunks. If the chunk takes up a whole region created by mmap, it will be unmapped. Otherwise, the free chunk is added to the correct bin of free chunks.
#Small Chunk Organisation
A normal chunk has a minimum size of 4 words (header, doubly linked list and footer). For small chunks, it is inefficient to use the data structures described above. Instead, small chunks are treated specially.
##Container
A container consists of space for multiple small chunks, a bitmask representing which chunks are in use, and a link to the next container. Chunks in a container are always the same size. A system of bins similar to that used by the large chunk allocator contain linked lists of containers with the same chunk size.
##Allocation Algorithm
Allocation is simple - the program finds a suitably sized container (creating one if necessary), finds a free chunk using the bitmap in the header and uses that.
##De-Allocation Algorithm
To de-allocate a chunk, the program checks all containers to see if they contain the chunk. When the container is found, the bitmask is modified to indicate that the chunk is now free. This approach is inefficient, though, as it requires traversing the containers. If the chunk is not there at all (which happens every time a large chunk is freed), every container must be visited.
#Future improvements
* Speed improvement - still slower than libc.
* minimise list traversals - splitting the lists into bins reduces traversal time, but with some work I could remove some of the O(n) traversals.