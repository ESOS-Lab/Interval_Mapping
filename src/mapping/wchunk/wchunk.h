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

#define WCHUNK_LENGTH_DIGIT 14
#define WCHUNK_BUCKET_DIGIT 4
#define WCHUNK_LENGTH (1 << WCHUNK_LENGTH_DIGIT)
#define WCHUNK_BUCKET_SIZE (1 << WCHUNK_BUCKET_DIGIT)
#define WCHUNK_CACHE_SIZE 20
#define WCHUNK_START_ADDR_MASK (~(WCHUNK_LENGTH - 1))
#define WCHUNK_BUCKET_INDEX_MASK \
    ((WCHUNK_BUCKET_SIZE - 1) << WCHUNK_LENGTH_DIGIT)
#define WCHUNK_BUCKET_INDEX(lsa) \
    ((lsa & WCHUNK_BUCKET_INDEX_MASK) >> WCHUNK_LENGTH_DIGIT)

#define WCHUNK_VALID_BIT_INDEX(index) (index >> 5)
#define WCHUNK_VALID_BIT_SELECTOR(index) (1 << (31 - (index & 0x1F)))

#define WCHUNK_ERASE_LIST_LENGTH (1024)

typedef struct wchunk {
    int numOfValidBits;  // number of valid bits, used for efficient decision of
                         // erase
    unsigned int validBits[WCHUNK_VALID_BIT_INDEX(WCHUNK_LENGTH)];
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

typedef struct wchunk_erase_list {
    WChunk_p wchunk_p[WCHUNK_ERASE_LIST_LENGTH];
    unsigned int wchunkStartAddr[WCHUNK_ERASE_LIST_LENGTH];
    int curItemCount;
} WChunkEraseList;

void wchunk_init();
unsigned int wchunk_get(WChunkBucket *wchunkBucket,
                        unsigned int logicalSliceAddr);
int wchunk_set(WChunkBucket *wchunkBucket, unsigned int logicalSliceAddr,
               unsigned int virtualSliceAddr);
int wchunk_set_range(WChunkBucket *wchunkBucket, unsigned int logicalSliceAddr,
                     int length, unsigned int virtualSliceAddr);
int wchunk_remove(WChunkBucket *wchunkBucket, unsigned int logicalSliceAddr);
int wchunk_remove_range(WChunkBucket *wchunkBucket,
                        unsigned int logicalSliceAddr, int length);

void wchunk_deallocate(WChunkCache *ccache, WChunk_p wchunk_p,
                       unsigned int chunkStartAddr);
int wchunk_is_valid(WChunkCache *ccache, WChunk_p wchunk_p,
                    unsigned int indexInChunk);
void wchunk_mark_valid(WChunkCache *ccache, WChunk_p wchunk_p,
                       unsigned int indexInChunk, int length,
                       unsigned int wchunkStartAddr, int isValid);

void wchunk_add_erase_chunk(WChunk_p wchunk_p, unsigned int wchunkStartAddr);
void wchunk_handle_erase(WChunkBucket *wchunkBucket);
extern WChunkBucket *wchunkBucket;

#endif /* SRC_MAPPING_WCHUNK_WCHUNK_H_ */
