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
#include "../../alex/openssd_allocator.h"

#define MAPSEG_CACHE_USE_LAST_SLOT 1

#define MAPSEG_LENGTH_DIGIT 10
#define MAPSEG_BUCKET_DIGIT 4
#define MAPSEG_MAP_SEGMENT_SIZE (1 << MAPSEG_LENGTH_DIGIT)
#define MAPSEG_BUCKET_SIZE (1 << MAPSEG_BUCKET_DIGIT)
#define MAPSEG_CACHE_SIZE 20
#define MAPSEG_START_ADDR_MASK (~(MAPSEG_MAP_SEGMENT_SIZE - 1))
#define MAPSEG_BUCKET_INDEX_MASK \
    ((MAPSEG_BUCKET_SIZE - 1) << MAPSEG_LENGTH_DIGIT)
#define MAPSEG_BUCKET_INDEX(lsa) \
    ((lsa & MAPSEG_BUCKET_INDEX_MASK) >> MAPSEG_LENGTH_DIGIT)

#define MAPSEG_VALID_BIT_INDEX(index) (index >> 3)
#define MAPSEG_VALID_BIT_SELECTOR(index, bitsInSlice) \
    (bitsInSlice << (28 - ((index & 0x7) << 2)))
#define MAPSEG_FULL_BITS_IN_SLICE 0xF

#define MAPSEG_ERASE_LIST_LENGTH (1024)

#define MAPSEG_INITIAL_BUFFER_SIZE 1

extern OpenSSDAllocator<unsigned int> rangeDirAllocator;
typedef struct range_directory {
    unsigned int *bufferIdxs;  // size = mappingSize/unitRangeSize,
                               // use this to find mapping in range buffer.
} RangeDirectory;

inline void mapseg_init_range_directory(RangeDirectory *rangeDir,
                                        unsigned int dirSize) {
    rangeDir->bufferIdxs = rangeDirAllocator.allocate(dirSize);
}

extern OpenSSDAllocator<LOGICAL_SLICE_ENTRY> rangeBufferAllocator;
typedef struct range_buffer {
    struct header {
        unsigned int startLsa : 16;
    };
    struct header header;
    LOGICAL_SLICE_ENTRY *entries;  // size = variable
} RangeBuffer;

inline void mapseg_init_range_buffer(RangeBuffer *rangeBuffer,
                                     unsigned int startLsa,
                                     unsigned int bufferSize) {
    rangeBuffer->header.startLsa = startLsa;
    rangeBuffer->entries = rangeBufferAllocator.allocate(bufferSize);
}

extern OpenSSDAllocator<RangeBuffer> mapSegmentBufferAllocator;
typedef struct map_segment {
    // metadata
    unsigned int startLsa;
    unsigned int mappingSize : 16;
    unsigned int unitRangeSize : 16;
    int numOfValidMaps : 16;  // number of valid bits, used for efficient
                             // decision of erase
    unsigned int validBits[MAPSEG_VALID_BIT_INDEX(MAPSEG_MAP_SEGMENT_SIZE)];

    RangeDirectory rangeDir;
    RangeBuffer *rangeBuffer;
} MapSegment, *MapSegment_p;

typedef struct map_segment_cache {
    MapSegment_p mapSegment_p[MAPSEG_CACHE_SIZE];
    unsigned int mapSegmentStartLsa[MAPSEG_CACHE_SIZE];
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

inline LOGICAL_SLICE_ENTRY *mapseg_fetch_entry_in_map_segment(
    MapSegment *pMapSegment, unsigned int logicalSliceAddr) {
    unsigned int dirIdx =
        (logicalSliceAddr - pMapSegment->startLsa) / pMapSegment->unitRangeSize;
    unsigned int bufferIdx = pMapSegment->rangeDir.bufferIdxs[dirIdx];

    unsigned int rangeStartLsa =
        pMapSegment->startLsa + pMapSegment->rangeBuffer->header.startLsa;
    unsigned int indexInMapSegment = logicalSliceAddr - rangeStartLsa;

    // xil_printf("entry fetched %p = %p\n", logicalSliceAddr,
    //            &pMapSegment->rangeBuffer->entries[indexInMapSegment]);
    return &pMapSegment->rangeBuffer->entries[indexInMapSegment];
}

void mapseg_init();
void mapseg_init_map_segment(MapSegment *mapSegment_p, unsigned int startLsa);
unsigned int mapseg_get_mapping(MapSegmentBucket *wchunkBucket,
                                unsigned int logicalSliceAddr);
int mapseg_set_mapping(MapSegmentBucket *wchunkBucket,
                       unsigned int logicalSliceAddr,
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
