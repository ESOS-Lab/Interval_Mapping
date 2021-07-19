/*
 * wchunk.h
 *
 *  Created on: 2021. 7. 16.
 *      Author: Minsu Jang (nobleminsu@gmail.com)
 */

#ifndef SRC_MAPPING_WCHUNK_WCHUNK_H_
#define SRC_MAPPING_WCHUNK_WCHUNK_H_

#include "../../address_translation.h"
#include "../../alex/alex.h"

#define WCHUNK_USE_LAST_SLOT 0

#define WCHUNK_LENGTH_DIGIT 16
#define WCHUNK_BUCKET_DIGIT 4
#define WCHUNK_LENGTH (1 << WCHUNK_LENGTH_DIGIT)
#define WCHUNK_BUCKET_SIZE (1 << WCHUNK_BUCKET_DIGIT)
#define WCHUNK_CACHE_SIZE 20
#define WCHUNK_START_ADDR_MASK (~(WCHUNK_LENGTH - 1))
#define WCHUNK_BUCKET_INDEX_MASK \
    ((WCHUNK_BUCKET_SIZE - 1) << WCHUNK_LENGTH_DIGIT)
#define WCHUNK_BUCKET_INDEX(lsa) \
    ((lsa & WCHUNK_BUCKET_INDEX_MASK) >> WCHUNK_LENGTH_DIGIT)

typedef struct wchunk {
    LOGICAL_SLICE_ENTRY entries[WCHUNK_LENGTH];
} WChunk, *WChunk_p;

using WChunkTree = alex::Alex<unsigned int, WChunk_p>;

typedef struct wchunk_cache {
    WChunk_p wchunk_p[WCHUNK_CACHE_SIZE];
    unsigned int wchunkStartAddr[WCHUNK_CACHE_SIZE];
    int lruValues[WCHUNK_CACHE_SIZE];
    int curItemCount;
    int maxLruValue;

#if WCHUNK_USE_LAST_SLOT
    int lastSelectedSlot;
#endif
} WChunkCache;

typedef struct wchunk_bucket {
    WChunkCache ccaches[WCHUNK_BUCKET_SIZE];
} WChunkBucket, *WChunkBucket_p;

void wchunk_init();
unsigned int wchunk_get(WChunkBucket *wchunkBucket,
                        unsigned int logicalSliceAddr);
int wchunk_set(WChunkBucket *wchunkBucket, unsigned int logicalSliceAddr,
               unsigned int virtualSliceAddr);
int wchunk_remove(WChunkBucket *wchunkBucket, unsigned int logicalSliceAddr);

extern WChunkBucket *wchunkBucket;

#endif /* SRC_MAPPING_WCHUNK_WCHUNK_H_ */
