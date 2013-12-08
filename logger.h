#ifndef VMEMALLOC_TRACER_GUARD
#define VMEMALLOC_TRACER_GUARD

// The total size (in bytes) of all currently allocated regions.
extern int allocatedSpace;

// The total number of currently allocated regions.
extern int allocatedChunkCount;

// The total size (in bytes) of all current free regions.
extern int freeSpace;

// The current number of free regions.
extern int freeChunkCount;

// The number of mmapped regions in use.
extern int regionsUsed;

// Outputs the headers for the tab seperated trace data columns.
extern void outputTraceData(char* op);

#endif
