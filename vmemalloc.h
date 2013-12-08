#include <stdlib.h>
#include <stdint.h>
#include <stdio.h>
#include <math.h>
#include <limits.h>

#include "logger.h"

// Assume Word has the same size as uintptr_t - it does on the lab machines.
typedef uintptr_t Word;

// Rounds up to a multiple.
#define CEIL(x, multiple) (((((x) - 1) / (multiple)) + 1) * (multiple))

#define SMALL_CHUNK_LIMIT_POWER 5
// chunks smaller than this are treated differently to minimise header size.
#define SMALL_CHUNK_LIMIT (1 << SMALL_CHUNK_LIMIT_POWER)

#define LARGEST_ALIGNMENT_TYPE long double

// Blocks from vmemalloc should be aligned to this.
#define LARGEST_ALIGNMENT sizeof(LARGEST_ALIGNMENT_TYPE)

// Makes sure all regions are aligned to LARGEST_ALIGNMENT.
#define ALIGNMENT_OFFSET (CEIL(sizeof(ChunkHeader), LARGEST_ALIGNMENT) - sizeof(ChunkHeader))


/*	Stuart Norcross - 12/03/10 */

//An interface for a heap alocation library


/*	Allocate 'size' bytes of memory. On success the function returns a pointer to 
	the start of the allocated region. On failure NULL is returned. */
extern void *vmemalloc(int size);

/*	Release the region of memory pointed to by 'ptr'. */
extern void vmemfree(void *ptr);

/*	Set the file specified by the 'file' parameter as the target for trace data. 
	If 'file' does not exist it will be created. 
	If this function is not called then no trace output should be generated.*/
extern void setTraceFile(char *file);

/*	Initialise the timing mechanism. */
extern void setupTimer(void);

/* Closes the trace file in use. Should be called at the end of execution. */
extern void closeTraceFile();
