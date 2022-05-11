/*
 * wchunk.c
 *
 *  Created on: 2021. 7. 16.
 *      Author: Minsu Jang (nobleminsu@gmail.com)
 */

#include "map_segment.h"

#include <assert.h>
#include <string.h>

#include "../../alex/openssd_allocator.h"
#include "../../ftl_config.h"
#include "../../memory_map.h"
#include "../functional/functional_mapping.h"
#include "xil_printf.h"
#include "xparameters.h"
#include "xtime_l.h"

char *ftableMemPool = (char *)RESERVED0_START_ADDR;

FunctionalMappingTree fmTrees[FUNCTIONAL_MAPPING_TREE_COUNT];
OpenSSDAllocator<DataNode> tAllocator;
OpenSSDAllocator<MapSegment> mapSegmentAllocator;
OpenSSDAllocator<unsigned int> rangeDirAllocator;
OpenSSDAllocator<LOGICAL_SLICE_ENTRY> rangeBufferAllocator;
OpenSSDAllocator<RangeBuffer> mapSegmentBufferAllocator;

int isFmTreesInitialized[FUNCTIONAL_MAPPING_TREE_COUNT];

MapSegment *mapseg_select_map_segment(unsigned int logicalSliceAddr,
                                      int isAllocate);
void wchunk_print_alex_stats();

void mapseg_init() {
    for (int i = 0; i < FUNCTIONAL_MAPPING_TREE_COUNT; i++) {
        initRootNode(&(fmTrees[i].rootNode), TREE_NUM_TO_FIRST_LSA(i));
        fetchMapSegmentFromFmTree(&fmTrees[i], TREE_NUM_TO_FIRST_LSA(i), 1);
        xil_printf("FmTree %d init: %p\n", i, TREE_NUM_TO_FIRST_LSA(i));

        isFmTreesInitialized[i] = 0;
    }
}

void mapseg_init_map_segment(MapSegment *pMapSegment, unsigned int startLsa) {
    pMapSegment->startLsa = startLsa;
    pMapSegment->mappingSize = MAPSEG_MAP_SEGMENT_SIZE;
    pMapSegment->unitRangeSize = MAPSEG_MAP_SEGMENT_SIZE;
    pMapSegment->numOfValidMaps = 0;
    memset(
        pMapSegment->validBits, 0,
        sizeof(unsigned int) * MAPSEG_VALID_BIT_INDEX(MAPSEG_MAP_SEGMENT_SIZE));

    unsigned int dirSize =
        pMapSegment->mappingSize / pMapSegment->unitRangeSize;
    // init directory
    mapseg_init_range_directory(&pMapSegment->rangeDir, dirSize);
    // init buffer
    pMapSegment->rangeBuffer =
        mapSegmentBufferAllocator.allocate(MAPSEG_INITIAL_BUFFER_SIZE);
    mapseg_init_range_buffer(pMapSegment->rangeBuffer, 0,
                             pMapSegment->mappingSize);
}

MapSegment *mapseg_select_map_segment(unsigned int logicalSliceAddr,
                                      int isAllocate) {
    FunctionalMappingTree *fmTree;
    MapSegment_p selectedMapSegment = NULL;

    fmTree = &fmTrees[LSA_TO_TREE_NUM(logicalSliceAddr)];
    if (isFmTreesInitialized[LSA_TO_TREE_NUM(logicalSliceAddr)] == 0) {
        isFmTreesInitialized[LSA_TO_TREE_NUM(logicalSliceAddr)] = 1;
        xil_printf("FmTree %d first access: %p\n",
                   LSA_TO_TREE_NUM(logicalSliceAddr), logicalSliceAddr);
    }
    selectedMapSegment =
        fetchMapSegmentFromFmTree(fmTree, logicalSliceAddr, isAllocate);

    return selectedMapSegment;
}

unsigned int mapseg_get_mapping(unsigned int logicalSliceAddr) {
    unsigned int virtualSliceAddr, selectedMapSegmentStartLsa,
        indexInMapSegment;
    MapSegment_p selectedMapSegment;

    selectedMapSegment = mapseg_select_map_segment(logicalSliceAddr, 0);
    if (selectedMapSegment == NULL) return VSA_FAIL;
    selectedMapSegmentStartLsa = selectedMapSegment->startLsa;
    indexInMapSegment = logicalSliceAddr - selectedMapSegmentStartLsa;

    if (!mapseg_is_valid(selectedMapSegment, indexInMapSegment)) {
        return VSA_FAIL;
    }

    virtualSliceAddr =
        mapseg_fetch_entry_in_map_segment(selectedMapSegment, logicalSliceAddr)
            ->virtualSliceAddr;

    return virtualSliceAddr;
}

int mapseg_set_mapping(unsigned int logicalSliceAddr,
                       unsigned int virtualSliceAddr) {
    unsigned int selectedMapSegmentStartAddr, indexInMapSegment;
    MapSegment_p selectedMapSegment;

    selectedMapSegment = mapseg_select_map_segment(logicalSliceAddr, 1);
    if (selectedMapSegment == NULL) return -1;
    selectedMapSegmentStartAddr = selectedMapSegment->startLsa;
    indexInMapSegment = logicalSliceAddr - selectedMapSegmentStartAddr;

    mapseg_fetch_entry_in_map_segment(selectedMapSegment, logicalSliceAddr)
        ->virtualSliceAddr = virtualSliceAddr;

    if (virtualSliceAddr != VSA_NONE)
        mapseg_mark_valid(selectedMapSegment, indexInMapSegment, 1,
                          selectedMapSegmentStartAddr, 1,
                          MAPSEG_FULL_BITS_IN_SLICE);
    else
        mapseg_mark_valid(selectedMapSegment, indexInMapSegment, 1,
                          selectedMapSegmentStartAddr, 0,
                          MAPSEG_FULL_BITS_IN_SLICE);
    return 0;
}

int mapseg_remove(unsigned int logicalSliceAddr) {
    return mapseg_set_mapping(logicalSliceAddr, VSA_NONE);
}

void mapseg_deallocate(MapSegment_p wchunk_p, unsigned int chunkStartAddr) {
    // TODO: implement fm deallocate
}

int mapseg_is_valid(MapSegment_p wchunk_p, unsigned int indexInChunk) {
    int validBitIndex, validBitSelector;
    validBitIndex = MAPSEG_VALID_BIT_INDEX(indexInChunk);
    validBitSelector =
        MAPSEG_VALID_BIT_SELECTOR(indexInChunk, MAPSEG_FULL_BITS_IN_SLICE);

    return wchunk_p->validBits[validBitIndex] & validBitSelector;
}

void mapseg_mark_valid(MapSegment_p wchunk_p, unsigned int indexInChunk,
                       int length, unsigned int wchunkStartAddr, int isValid,
                       int bitsInSlice) {
    int validBitIndex, validBitSelector, origBits, newBits = 0;
    for (int i = 0; i < length; i++) {
        validBitIndex = MAPSEG_VALID_BIT_INDEX((indexInChunk + i));
        validBitSelector =
            MAPSEG_VALID_BIT_SELECTOR((indexInChunk + i), bitsInSlice);

        origBits = wchunk_p->validBits[validBitIndex];

        if (isValid) {
            newBits = origBits | validBitSelector;
        } else {
            newBits = origBits & (~validBitSelector);
        }
        wchunk_p->numOfValidMaps +=
            __builtin_popcountl(newBits) - __builtin_popcountl(origBits);
        wchunk_p->validBits[validBitIndex] = newBits;
    }

    // deallocate totally unused chunk
    if (wchunk_p->numOfValidMaps == 0)
        mapseg_deallocate(wchunk_p, wchunkStartAddr);
}

int mapseg_mark_valid_partial(unsigned int logicalSliceAddr, int isValid,
                              int start, int end) {
    unsigned int selectedChunkStartAddr, indexInChunk;
    MapSegment_p selectedChunk;
    int bitsInSlice;

    selectedChunk = mapseg_select_map_segment(logicalSliceAddr, 1);
    if (selectedChunk == NULL) return 0;
    selectedChunkStartAddr = selectedChunk->startLsa;
    indexInChunk = logicalSliceAddr - selectedChunkStartAddr;

    bitsInSlice = 0;
    for (int i = start; i < end; i++) {
        bitsInSlice += (1 << (3 - i));
    }
    mapseg_mark_valid(selectedChunk, indexInChunk, 1, selectedChunkStartAddr,
                      isValid, bitsInSlice);

    // return 1 if the entry is totally invalidated, else 0
    return !mapseg_is_valid(selectedChunk, indexInChunk);
}
