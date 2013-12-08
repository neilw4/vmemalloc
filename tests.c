#include <stdio.h>
#include <assert.h>
#include <string.h>

#include "vmemalloc.h"


#define NUM_TO_ALLOC 65

void checkBlock(unsigned char* block, unsigned char value, size_t size) {
    for (int i = 0; i < (int)size; i++) {
        assert(block[0] == value);
    }
}

void testLarge() {
    printf("Testing large allocations\n");

    assert(allocatedSpace == 0);
    assert(allocatedChunkCount == 0);
    assert(freeChunkCount == 0);
    assert(regionsUsed == 0);

    unsigned char* allocd[NUM_TO_ALLOC];

    allocd[0] = vmemalloc(70);

    assert(allocd[0] != NULL);
    memset(allocd[0], 1, 70);

    assert(allocatedSpace >= 70);
    assert(allocatedChunkCount == 1);
    assert(freeChunkCount <= 1);
    assert(regionsUsed == 1);

    vmemfree(allocd[0]);

    assert(allocatedSpace == 0);
    assert(allocatedChunkCount == 0);
    assert(freeChunkCount == 0);
    assert(regionsUsed == 0);

    int sizeToAlloc = 70;
    for (int i = 0; i < NUM_TO_ALLOC; i++) {
        allocd[i] = vmemalloc(sizeToAlloc);
        assert(allocd[i] != NULL);
        memset(allocd[i], i, sizeToAlloc);
        sizeToAlloc += 70;
    }

    sizeToAlloc = 70;
    for (int i = 0; i < NUM_TO_ALLOC; i++) {
        // Check that nothing has changed.
        for (int j = 0; j < sizeToAlloc; j++) {
            assert(allocd[i][j] == i);
        }
        vmemfree(allocd[i]);
        sizeToAlloc += 70;
    }
    assert(allocatedSpace == 0);
    assert(allocatedChunkCount == 0);
    assert(freeChunkCount == 0);
    assert(regionsUsed == 0);
}


void testSmall() {
    printf("Testing small allocations\n");

    char *chars[NUM_TO_ALLOC];
    long *longs[NUM_TO_ALLOC];

    chars[0] = vmemalloc(sizeof(char));

    assert(chars[0] != NULL);
    *chars[0] = (char)~0;
    assert(allocatedSpace >= (int)sizeof(char));
    assert(allocatedChunkCount == 1);
    assert(freeChunkCount <= 1);
    assert(regionsUsed == 1);

    vmemfree(chars[0]);

    assert(allocatedSpace == 0);
    assert(allocatedChunkCount == 0);
    assert(regionsUsed == 0);
    assert(freeChunkCount == 0);

    for (int i = 0; i < NUM_TO_ALLOC; i++) {
        chars[i] = vmemalloc(sizeof(char));
        *chars[i] = (char)i;
        longs[i] = vmemalloc(sizeof(long));
        *longs[i] = (long)i;
    }

    assert(allocatedSpace >= NUM_TO_ALLOC * (int)(sizeof(char) + sizeof(long)));
    assert(allocatedChunkCount > 0);
    assert(regionsUsed > 0);

    int middle = NUM_TO_ALLOC / 2;

    for (int i = middle; i < NUM_TO_ALLOC; i++) {
        assert(*chars[i] == (char)i);
        vmemfree(chars[i]);
        assert(*longs[i] == (long)i);
        vmemfree(longs[i]);
    }

    for (int i = middle - 1; i >= 0; i--) {
        assert(*chars[i] == (char)i);
        vmemfree(chars[i]);
        assert(*longs[i] == (long)i);
        vmemfree(longs[i]);
    }

    assert(allocatedSpace == 0);
    assert(allocatedChunkCount == 0);
    assert(regionsUsed == 0);
    assert(freeChunkCount == 0);
}

int main(){
    setupTimer();
    setTraceFile("experiment2.csv");

    testLarge();
    testSmall();

    closeTraceFile();
    printf("Test succeeded.\n");
    return 0;
}
