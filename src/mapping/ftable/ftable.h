/*
 * ftable.h
 *
 *  Created on: 2021. 4. 26.
 *      Author: Minsu Jang (nobleminsu@gmail.com)
 */

#ifndef SRC_MAPPING_FTABLE_FTABLE_H_
#define SRC_MAPPING_FTABLE_FTABLE_H_

#include "../../address_translation.h"

#define FTABLE_DEFAULT_TABLE_NUM 8
#define FTABLE_DEFAULT_CAPACITY 12800 * 100
#define FTABLE_DEFAULT_AFTER_SLIDE_RATIO 0.2
#define FTABLE_DEFAULT_INVALIDATED_SLIDE_THR_RATIO 0.8

#define FTABLE_ENTRY_NOT_COVERED 0
#define FTABLE_ENTRY_ACTIVE 1
#define FTABLE_ENTRY_SLIDED 2

typedef struct ftable {
    LOGICAL_SLICE_ENTRY *entries;
    size_t capacity;
    unsigned int initialHeadAddr;  // to check whether given address is fresh or
                                   // already slided

    unsigned int headIndex;  // start of index for head
    size_t filledBeforeNextSlideHead;
    size_t filledAfterNextSlideHead;
    size_t invalidatedBeforeNextSlideHead;
    size_t invalidatedAfterNextSlideHead;

    float
        afterSlideRatio;  // filled/capacity ratio that will make table to slide
    float invalidatedSlideThresholdRatio;  // ratio of invalidated entries that
                                           // will make table to slide

    unsigned int focusingHeadAddr;  // current head, where headIndex points to
                                    // the entry of this value
    unsigned int mappingUnit;       // unit of mapping for an entry
} FTable;

FTable *ftable_create_table(unsigned int focusingHeadAddr, FTable ftables[],
                            int *curMaxFTableIdx, int maxFTableIndex);
int ftable_insert(FTable *ftable, unsigned int logicalSliceAddr,
                  unsigned int virtualSliceAddr);
int ftable_get(FTable *ftable, unsigned int sliceAddr);
int ftable_invalidate(FTable *ftable, unsigned int sliceAddr);
FTable *ftable_select_table(unsigned int sliceAddr, FTable ftables[],
                            int curMaxFTableIdx);
int ftable_get_entry_state(unsigned int sliceAddr, FTable ftables[],
                           int tableLength);

#endif /* SRC_MAPPING_FTABLE_FTABLE_H_ */
