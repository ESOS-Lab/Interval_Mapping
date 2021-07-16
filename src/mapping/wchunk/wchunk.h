/*
 * ftable.h
 *
 *  Created on: 2021. 4. 26.
 *      Author: Minsu Jang (nobleminsu@gmail.com)
 */

#ifndef SRC_MAPPING_FTABLE_FTABLE_H_
#define SRC_MAPPING_FTABLE_FTABLE_H_

#include "../../address_translation.h"
#include "../../alex/alex.h"

#define WCHUNK_USE_LAST_SLOT 0

#define WCHUNK_LENGTH_DIGIT 10
#define WCHUNK_LENGTH (1 << WCHUNK_LENGTH_DIGIT)
#define WCHUNK_CACHE_SIZE 20
#define WCHUNK_CHUNK_SIZE_MASK (~(WCHUNK_LENGTH - 1))

#define FTABLE_DEBUG 1

// typedef struct ftable {
//     LOGICAL_SLICE_ENTRY *entries;
//     size_t capacity;
//     unsigned int initialHeadAddr;  // to check whether given address is fresh
//     or
//                                    // already slided

//     unsigned int headIndex;  // start of index for head
//     size_t filledBeforeNextSlideHead;
//     size_t filledAfterNextSlideHead;
//     size_t invalidatedBeforeNextSlideHead;
//     size_t invalidatedAfterNextSlideHead;

//     float
//         afterSlideRatio;  // filled/capacity ratio that will make table to
//         slide
//     float invalidatedSlideThresholdRatio;  // ratio of invalidated entries
//     that
//                                            // will make table to slide

//     unsigned int focusingHeadAddr;  // current head, where headIndex points
//     to
//                                     // the entry of this value
// } FTable;

typedef struct wchunk {
    LOGICAL_SLICE_ENTRY entries[WCHUNK_LENGTH];
} WChunk, *WChunk_p;

using WChunkTree = alex::Alex<unsigned int, WChunk_p>;
// typedef struct wchunk_tree {
//    alex::Alex<unsigned int, WChunk_p> tree;
//} WChunkTree;

typedef struct wchunk_cache {
    WChunk_p wchunk_p[WCHUNK_CACHE_SIZE];
    unsigned int wchunkStartAddr[WCHUNK_CACHE_SIZE];
    unsigned int lruValues[WCHUNK_CACHE_SIZE];
    int curItemCount;
    int maxLruValue;

#if WCHUNK_USE_LAST_SLOT
    int lastSelectedSlot;
#endif
    // union {
    //     unsigned long long qword;
    //     char isNewlyAllocated[WCHUNK_CACHE_SIZE];
    // };
} WChunkCache;

void wchunk_init();
unsigned int wchunk_get(WChunkCache *ccache, unsigned int logicalSliceAddr);
int wchunk_set(WChunkCache *ccache, unsigned int logicalSliceAddr,
               unsigned int virtualSliceAddr);
int wchunk_remove(WChunkCache *ccache, unsigned int logicalSliceAddr);

// FTable *ftable_create_table(unsigned int focusingHeadAddr, FTable ftables[],
//                             int *curMaxFTableIdx, int maxFTableIndex);
// int ftable_insert(FTable *ftable, unsigned int logicalSliceAddr,
//                   unsigned int virtualSliceAddr,
//                   void (*migrationHandler)(unsigned int, unsigned int));
// int ftable_get(FTable *ftable, unsigned int sliceAddr);
// int ftable_invalidate(FTable *ftable, unsigned int sliceAddr,
//                       void (*migrationHandler)(unsigned int, unsigned int));
// FTable *ftable_select_table(unsigned int sliceAddr, FTable ftables[],
//                             int curMaxFTableIdx);
// int ftable_get_entry_state(unsigned int sliceAddr, FTable ftables[],
//                            int tableLength);
// void ftable_update(FTable *ftable, unsigned int logicalSliceAddr,
//                    unsigned int virtualSliceAddr);

extern WChunkCache *ccache;

#endif /* SRC_MAPPING_FTABLE_FTABLE_H_ */
