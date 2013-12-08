#include <stdio.h>
#include <stdlib.h>
#include <sys/time.h>

#include "logger.h"


/* Adapted from code by Stuart Norcross */

// This is to identify the fist call to output the trace data (first call to vmemfree or vmemalloc).
// Before any trace data is written the column headers are written. Set to 0 when trace data is output.
static int firstCall = 1;

// The output stream to which trace data will be written.
static FILE* traceFile = NULL;

static long start_time;

// The total size (in bytes) of all currently allocated regions.
int allocatedSpace = 0;

// The total number of currently allocated regions.
int allocatedChunkCount = 0;

// The total size (in bytes) of all current free regions.
int freeSpace = 0;

// The current number of free regions.
int freeChunkCount = 0;

// The number of mmapped regions in use.
int regionsUsed = 0;

// Current time in microseconds.
long time_in_usecs() {
    struct timeval tim;
    gettimeofday(&tim, NULL);
    long current_time = tim.tv_sec * 1000000 + tim.tv_usec;
    return current_time;
}

// Execution time in micro-seconds.
long execution_time() {
    long current_time = time_in_usecs();
    return current_time-start_time;
}

void setupTimer() {
    start_time = time_in_usecs();
}

// Outputs the headers for the tab seperated trace data columns.
void outputTraceColumnHeaders() {
    fprintf(traceFile, "Function,Time(us),Alloc'd Space,Alloc'd Chunks,Free Space,Free Chunks,Regions\n");
}


void outputTraceData(char* op) {
    // Only output trace data if the trace file has been set.
    if (traceFile != NULL) {
        if (firstCall) {
            firstCall = 0;
            start_time = time_in_usecs();
            outputTraceColumnHeaders();
        }
        fprintf(traceFile, "%s,%ld,%d,%d,%d,%d,%d\n", op, execution_time(), allocatedSpace,
                allocatedChunkCount, freeSpace, freeChunkCount, regionsUsed);
    }
}

// Set the trace file. If path is NULL then trace is output to stdout.
void setTraceFile(char* path) {
    if (traceFile == NULL) {
        if (path != NULL) {
            if ((traceFile = fopen(path, "a+")) == NULL) {
                perror("Failed to open tracefile:");
                exit(EXIT_FAILURE);
            }
        } else {
            traceFile = stdout;
        }
        
    } else {
        fprintf(stderr, "Trace file cannot be set twice. Exiting.\n");
        exit(EXIT_FAILURE);
    }
}

void closeTraceFile() {
    fclose(traceFile);
    traceFile = NULL;
}
