/*
 * wchunk.h
 *
 *  Created on: 2021. 7. 16.
 *      Author: Minsu Jang (nobleminsu@gmail.com)
 */

#ifndef SRC_MAPPING_MAPSEG_MAP_SEGMENT_H_
#define SRC_MAPPING_MAPSEG_MAP_SEGMENT_H_

#include "../../address_translation.h"
#include "../../alex/alex.h"

#define MAPSEG_CACHE_USE_LAST_SLOT 1

#define MAPSEG_LENGTH_DIGIT 10
#define MAPSEG_BUCKET_DIGIT 4
#define MAPSEG_UNIT_RANGE_SIZE (1 << MAPSEG_LENGTH_DIGIT)
#define MAPSEG_BUCKET_SIZE (1 << MAPSEG_BUCKET_DIGIT)
#define MAPSEG_CACHE_SIZE 20
#define MAPSEG_START_ADDR_MASK (~(MAPSEG_UNIT_RANGE_SIZE - 1))
#define MAPSEG_BUCKET_INDEX_MASK \
    ((MAPSEG_BUCKET_SIZE - 1) << MAPSEG_LENGTH_DIGIT)
#define MAPSEG_BUCKET_INDEX(lsa) \
    ((lsa & MAPSEG_BUCKET_INDEX_MASK) >> MAPSEG_LENGTH_DIGIT)

#define MAPSEG_VALID_BIT_INDEX(index) (index >> 3)
#define MAPSEG_VALID_BIT_SELECTOR(index, bitsInSlice) \
    (bitsInSlice << (28 - ((index & 0x7) << 2)))
#define MAPSEG_FULL_BITS_IN_SLICE 0xF

#define MAPSEG_ERASE_LIST_LENGTH (1024)

typedef struct map_segment {
    unsigned int startLba : 4;
    unsigned int mappingSize : 2;
    unsigned int unitRangeSize : 2;
    int numOfValidMaps : 2;  // number of valid bits, used for efficient
                             // decision of erase
    unsigned int validBits[MAPSEG_VALID_BIT_INDEX(MAPSEG_UNIT_RANGE_SIZE)];
    LOGICAL_SLICE_ENTRY entries[MAPSEG_UNIT_RANGE_SIZE];
} MapSegment, *MapSegment_p;

typedef struct map_segment_cache {
    MapSegment_p mapSegment_p[MAPSEG_CACHE_SIZE];
    unsigned int mapSegmentStartAddr[MAPSEG_CACHE_SIZE];
    int lruValues[MAPSEG_CACHE_SIZE];
    int curItemCount;
    int maxLruValue;

#if MAPSEG_CACHE_USE_LAST_SLOT
    int lastSelectedSlot;
#endif
} MapSegmentCache;

typedef struct map_segment_bucket {
    MapSegmentCache mapSegmentCaches[MAPSEG_BUCKET_SIZE];
} MapSegmentBucket, *MapSegmentBucket_p;

typedef struct map_segment_erase_list {
    MapSegment_p mapSegment_p[MAPSEG_ERASE_LIST_LENGTH];
    unsigned int mapSegmentStartAddr[MAPSEG_ERASE_LIST_LENGTH];
    int curItemCount;
} MapSegmentEraseList;

void mapseg_init();
unsigned int mapseg_get(MapSegmentBucket *wchunkBucket,
                        unsigned int logicalSliceAddr);
int mapseg_set(MapSegmentBucket *wchunkBucket, unsigned int logicalSliceAddr,
               unsigned int virtualSliceAddr);
int mapseg_set_range(MapSegmentBucket *wchunkBucket,
                     unsigned int logicalSliceAddr, int length,
                     unsigned int virtualSliceAddr);
int mapseg_remove(MapSegmentBucket *wchunkBucket,
                  unsigned int logicalSliceAddr);
int mapseg_remove_range(MapSegmentBucket *wchunkBucket,
                        unsigned int logicalSliceAddr, int length);

void mapseg_deallocate(MapSegmentCache *ccache, MapSegment_p wchunk_p,
                       unsigned int chunkStartAddr);
int mapseg_is_valid(MapSegmentCache *ccache, MapSegment_p wchunk_p,
                    unsigned int indexInChunk);
void mapseg_mark_valid(MapSegmentCache *ccache, MapSegment_p wchunk_p,
                       unsigned int indexInChunk, int length,
                       unsigned int wchunkStartAddr, int isValid,
                       int bitsInSlice);
int mapseg_mark_valid_partial(MapSegmentBucket *wchunkBucket,
                              unsigned int logicalSliceAddr, int isValid,
                              int start, int end);

void mapseg_add_erase_chunk(MapSegment_p wchunk_p,
                            unsigned int wchunkStartAddr);
void mapseg_handle_erase(MapSegmentBucket *wchunkBucket);
extern MapSegmentBucket *wchunkBucket;

#endif /* SRC_MAPPING_MAPSEG_MAP_SEGMENT_H_ */
