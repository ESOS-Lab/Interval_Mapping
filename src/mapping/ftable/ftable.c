/*
 * ftable.c
 *
 *  Created on: 2021. 4. 26.
 *      Author: Minsu Jang (nobleminsu@gmail.com)
 */

#include "ftable.h"

#include <assert.h>

#include "../../ftl_config.h"
#include "../../memory_map.h"
#include "xil_printf.h"

char *ftableMemPool = (char *)RESERVED0_START_ADDR;

// FTable ftables[FTABLE_TABLE_NUM];
// int curMaxFTableIdx = -1;

int ftable_addr_to_raw_index(FTable *ftable, unsigned int sliceAddr);
void ftable_slide(FTable *ftable);
unsigned int ftable_get_next_slide_head_addr(FTable *ftable);

FTable *ftable_create_table(unsigned int focusingHeadAddr, FTable ftables[],
                            int *curMaxFTableIdx, int maxFTableIndex) {
    *curMaxFTableIdx = *curMaxFTableIdx + 1;
    if (*curMaxFTableIdx >= maxFTableIndex) {
        assert(!"Cannot create more tables. Please increase maxFTableIndex.");
    }

    ftables[*curMaxFTableIdx].capacity = FTABLE_DEFAULT_CAPACITY;
    ftables[*curMaxFTableIdx].initialHeadAddr = focusingHeadAddr;

    ftables[*curMaxFTableIdx].headIndex = 0;
    ftables[*curMaxFTableIdx].filledBeforeNextSlideHead = 0;
    ftables[*curMaxFTableIdx].filledAfterNextSlideHead = 0;
    ftables[*curMaxFTableIdx].invalidatedBeforeNextSlideHead = 0;
    ftables[*curMaxFTableIdx].invalidatedAfterNextSlideHead = 0;

    ftables[*curMaxFTableIdx].afterSlideRatio =
        FTABLE_DEFAULT_AFTER_SLIDE_RATIO;
    ftables[*curMaxFTableIdx].invalidatedSlideThresholdRatio =
        FTABLE_DEFAULT_INVALIDATED_SLIDE_THR_RATIO;
    ftables[*curMaxFTableIdx].focusingHeadAddr = focusingHeadAddr;

    ftables[*curMaxFTableIdx].entries = (LOGICAL_SLICE_ENTRY *)ftableMemPool;
    ftableMemPool +=
        ftables[*curMaxFTableIdx].capacity * sizeof(LOGICAL_SLICE_ENTRY);

    int i;
    for (i = 0; i < ftables[*curMaxFTableIdx].capacity; i++) {
        ftables[*curMaxFTableIdx].entries[i].virtualSliceAddr = VSA_NONE;
    }

    xil_printf("ftable created for %p, ftableMemPool is now %p\n",
               ftables[*curMaxFTableIdx].entries, ftableMemPool);

    return &ftables[*curMaxFTableIdx];
}

int ftable_insert(FTable *ftable, unsigned int logicalSliceAddr,
                  unsigned int virtualSliceAddr) {
    unsigned int index = ftable_addr_to_raw_index(ftable, logicalSliceAddr);
    if (index < 0) assert(!"index is not valid for FTable");
    // if entry is newly written, increment filled count
    if (ftable->entries[index].virtualSliceAddr == VSA_NONE) {
        unsigned int nextSlideHeadAddr =
            ftable_get_next_slide_head_addr(ftable);
        if (logicalSliceAddr < nextSlideHeadAddr)
            ftable->filledBeforeNextSlideHead++;
        else
            ftable->filledAfterNextSlideHead;
    }

    ftable->entries[index].virtualSliceAddr = virtualSliceAddr;

    // if (FTABLE_DEBUG)
    //     xil_printf("ftable insert logical=%p, virtual=%p, insertedTo=%d\n",
    //                logicalSliceAddr, virtualSliceAddr, index);

    // if table is almost filled, slide
    // this assumes that sliceAddrs are accessed linearly
    if (logicalSliceAddr >=
        ftable->focusingHeadAddr +
            (ftable->capacity) * FTABLE_DEFAULT_INSERT_SLIDE_THRE_RATIO) {
        ftable_slide(ftable);
    }
}

int ftable_get(FTable *ftable, unsigned int sliceAddr) {
    unsigned int index = ftable_addr_to_raw_index(ftable, sliceAddr);
    if (index < 0) assert(!"index is not valid for FTable");
    return ftable->entries[index].virtualSliceAddr;
}

int ftable_invalidate(FTable *ftable, unsigned int sliceAddr) {
    unsigned int index = ftable_addr_to_raw_index(ftable, sliceAddr);
    if (index < 0) assert(!"index is not valid for FTable");

    if (ftable->entries[index].virtualSliceAddr != VSA_NONE) {
        unsigned int nextSlideHeadAddr =
            ftable_get_next_slide_head_addr(ftable);
        if (sliceAddr < nextSlideHeadAddr)
            ftable->invalidatedBeforeNextSlideHead++;
        else
            ftable->invalidatedAfterNextSlideHead++;

        ftable->entries[index].virtualSliceAddr = VSA_NONE;
    } else
        assert(!"trying to invalidate unmapped entry");

    // if ratio of the invalidated entries before the next slide head addr
    // exceeds the threshold, slide FTable.
    if ((float)ftable->invalidatedBeforeNextSlideHead /
            (ftable->capacity * (1 - ftable->afterSlideRatio)) >
        ftable->invalidatedSlideThresholdRatio) {
        ftable_slide(ftable);
    }
    return 0;
}

// Select a table that contains sliceAddr translation data.
// If not, create one table and put data in it.
FTable *ftable_select_table(unsigned int sliceAddr, FTable ftables[],
                            int curMaxFTableIdx) {
    int i;
    for (i = 0; i <= curMaxFTableIdx; i++) {
        if (ftable_addr_to_raw_index(&ftables[i], sliceAddr) > -1) {
            return &ftables[i];
        }
    }
    return NULL;
}

// Convert given addr to the index in the FTable.
int ftable_addr_to_raw_index(FTable *ftable, unsigned int sliceAddr) {
    unsigned index = ftable->headIndex + (sliceAddr - ftable->focusingHeadAddr);

    if (index > ftable->headIndex + ftable->capacity) {
        return -1;
    }

    // if index exceeds the capacity, start from the top.
    if (index > ftable->capacity) {
        return index - ftable->capacity;
    }
    return index;
}

// Slide focusingHead at address which is 'afterSlideRatio' ahead from the max
// address.
// Triggered by two condition
// 1. Table is almost filled.
// 2. The number of invalidated items exceeds certain threshold.
void ftable_slide(FTable *ftable) {
    unsigned int slidedHeadAddr = ftable_get_next_slide_head_addr(ftable);
    unsigned int nextHeadIndex =
        ftable_addr_to_raw_index(ftable, slidedHeadAddr);

    ftable->focusingHeadAddr = slidedHeadAddr;
    ftable->headIndex = nextHeadIndex;

    ftable->filledBeforeNextSlideHead = ftable->filledAfterNextSlideHead;
    ftable->invalidatedBeforeNextSlideHead =
        ftable->invalidatedAfterNextSlideHead;
    // migrate invalidated count

    // handle evicted entries
}

unsigned int ftable_get_next_slide_head_addr(FTable *ftable) {
    return ftable->focusingHeadAddr +
           ftable->capacity * (1 - ftable->afterSlideRatio);
}

int ftable_get_entry_state(unsigned int sliceAddr, FTable ftables[],
                           int tableLength) {
    int i;
    for (i = 0; i < tableLength; i++) {
        if (ftables[i].initialHeadAddr <= sliceAddr &&
            sliceAddr < ftables[i].focusingHeadAddr) {
            return FTABLE_ENTRY_SLIDED;
        } else if (ftables[i].focusingHeadAddr <= sliceAddr &&
                   sliceAddr <
                       ftables[i].focusingHeadAddr + ftables[i].capacity) {
            return FTABLE_ENTRY_ACTIVE;
        }
    }
    return FTABLE_ENTRY_NOT_COVERED;
}