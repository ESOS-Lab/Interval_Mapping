/*
 * focusing_table.c
 *
 *  Created on: 2021. 4. 26.
 *      Author: Minsu Jang (nobleminsu@gmail.com)
 */

#include "focusing_table.h"

#include <assert.h>

#include "../ftl_config.h"
#include "xil_printf.h"

char *ftableMemPool = RESERVED0_START_ADDR;

FTable focusingTable[FTABLE_TABLE_NUM];
int curMaxFocusingTableIdx = -1;

int ftable_addr_to_raw_index(FTable *ftable, unsigned int sliceAddr);
void ftable_slide(unsigned int slideHeadAddr);
unsigned int ftable_get_next_slide_head_addr(Ftable *ftable);

FTable *ftable_create(unsigned int focusingHeadAddr) {
    curMaxFocusingTableIdx++;
    if (curMaxFocusingTableIdx >= FTABLE_TABLE_NUM) {
        assert(!"Cannot create more tables. Please increase FTABLE_TABLE_NUM.");
    }

    focusingTable[curMaxFocusingTableIdx].capacity = FTABLE_DEFAULT_CAPACITY;

    focusingTable[curMaxFocusingTableIdx].headIndex = 0;
    focusingTable[curMaxFocusingTableIdx].filled = 0;
    focusingTable[curMaxFocusingTableIdx].invalidatedBeforeNextSlideHead = 0;
    focusingTable[curMaxFocusingTableIdx].invalidatedAfterNextSlideHead = 0;

    focusingTable[curMaxFocusingTableIdx].afterSlideRatio =
        FTABLE_DEFAULT_AFTER_SLIDE_RATIO;
    focusingTable[curMaxFocusingTableIdx].invalidatedSlideThresholdRatio =
        FTABLE_DEFAULT_INVALIDATED_SLIDE_THR_RATIO;
    focusingTable[curMaxFocusingTableIdx].focusingHeadAddr = focusingHeadAddr;
    focusingTable[curMaxFocusingTableIdx].mappingUnit =
        BYTES_PER_DATA_REGION_OF_SLICE;

    focusingTable[curMaxFocusingTableIdx].entries = ftableMemPool;
    ftableMemPool += focusingTable[curMaxFocusingTableIdx].capacity *
                     sizeof(LOGICAL_SLICE_ENTRY);

    int i;
    for (i = 0; i < focusingTable[curMaxFocusingTableIdx].capacity; i++) {
        focusingTable[curMaxFocusingTableIdx].entries[i].virtualSliceAddr =
            VSA_NONE;
    }

    xil_printf("ftable created for %p, ftableMemPool is now %p\n",
               focusingTable[curMaxFocusingTableIdx].entries, ftableMemPool);

    return &focusingTable[curMaxFocusingTableIdx];
}

int ftable_insert(FTable *ftable, unsigned int logicalSliceAddr,
                  unsigned int virtualSliceAddr) {
    unsigned int index = ftable_addr_to_raw_index(ftable, logicalSliceAddr);
    if (index < 0) assert(!"index is not valid for FTable");
    ftable->entries[index].virtualSliceAddr = virtualSliceAddr;

    if (index == ftable->capacity) {
        ftable_slide(ftable);
    }
}

int ftable_get(FTable *ftable, unsigned int sliceAddr) {
    unsigned int index = ftable_addr_to_raw_index(ftable, logicalSliceAddr);
    if (index < 0) assert(!"index is not valid for FTable");
    return ftable->entries[index].virtualSliceAddr;
}

int ftable_invalidate(FTable *ftable, unsigned int sliceAddr) {
    unsigned int index = ftable_addr_to_raw_index(ftable, sliceAddr);
    if (index < 0) assert(!"index is not valid for FTable");
    ftable->entries[index].virtualSliceAddr = VSA_NONE;

    unsigned int nextSlideHeadAddr = ftable_get_next_slide_head_addr(ftable);
    if (sliceAddr < nextSlideHeadAddr)
        ftable->invalidatedBeforeNextSlideHead++;
    else
        ftable->invalidatedAfterNextSlideHead++;

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
int ftable_select_or_create_table(unsigned int sliceAddr) {
    int i;
    for (i = 0; i <= curMaxFocusingTableIdx; i++) {
        if (ftable_addr_to_raw_index(focusingTable[i], sliceAddr) > -1) {
            return focusingTable[i];
        }
    }
    return ftable_create(sliceAddr);
}

// Convert given addr to the index in the FTable.
int ftable_addr_to_raw_index(FTable *ftable, unsigned int sliceAddr) {
    unsigned index =
        (sliceAddr - ftable->focusingHeadAddr) / ftable->mappingUnit;

    if (index > ftable->headIndex + ftable->capacity) {
        return -1;
    }

    if (index > ftable->capacity) {
        return index - ftable->capacity;
    }
    return index;
}

// Slide focusingHead at address which is 'afterSlideRatio' ahead from the max
// address.
// Triggered by two condition
// 1. Table is completely filled.
// 2. The number of invalidated items exceeds certain threshold.
void ftable_slide(FTable *ftable) {
    unsigned int slidedHeadAddr = ftable_get_next_slide_head_addr(ftable);
    ftable->headIndex = ftable_addr_to_raw_index(ftable, slidedHeadAddr);
    ftable->focusingHeadAddr = slideHeadAddr;
    ftable->invalidatedBeforeNextSlideHead = invalidatedAfterNextSlideHead;

    // handle evicted entries
}

unsigned int ftable_get_next_slide_head_addr(Ftable *ftable) {
    return ftable->focusingHeadAddr + ftable->mappingUnit * ftable->capacity *
                                          (1 - ftable->afterSlideRatio);
}