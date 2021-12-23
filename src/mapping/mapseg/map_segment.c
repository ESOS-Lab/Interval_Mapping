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

// WChunkCache *ccache;
MapSegmentBucket *wchunkBucket;
MapSegmentEraseList wchunkEraseList;
FunctionalMappingTree fmTrees[FUNCTIONAL_MAPPING_TREE_COUNT];
OpenSSDAllocator<TempNode> tAllocator;
OpenSSDAllocator<MapSegment> cAllocator;

int isFmTreesInitialized[FUNCTIONAL_MAPPING_TREE_COUNT];

int wchunk_get_lru_slot(MapSegmentCache *ccache);
void wchunk_mark_mru(MapSegmentCache *ccache, int slot);
int wchunk_select_chunk(MapSegmentCache *ccache, unsigned int logicalSliceAddr,
                        int isAllocate);
MapSegment_p wchunk_allocate_new(MapSegmentCache *ccache, unsigned int chunkStartAddr);
void wchunk_print_alex_stats();

void mapseg_init() {
    MapSegmentCache *ccache;

    OpenSSDAllocator<MapSegmentBucket> aa;
    wchunkBucket = (MapSegmentBucket_p)aa.allocate(1);

    for (int i = 0; i < MAPSEG_BUCKET_SIZE; i++) {
        ccache = &wchunkBucket->mapSegmentCaches[i];
        ccache->curItemCount = 0;
        ccache->maxLruValue = 0;

#if WCHUNK_USE_LAST_SLOT
        ccache->lastSelectedSlot = -1;
#endif
    }

    for (int i = 0; i < FUNCTIONAL_MAPPING_TREE_COUNT; i++) {
        initRootNode(&(fmTrees[i].rootNode), TREE_NUM_TO_FIRST_LSA(i));
        fetchChunkFromFmTree(&fmTrees[i], TREE_NUM_TO_FIRST_LSA(i), 1);
        xil_printf("FmTree %d init: %p\n", i, TREE_NUM_TO_FIRST_LSA(i));

        isFmTreesInitialized[i] = 0;
    }
}

// XTime lastReportTime;
// int calls = 0;
// XTime totalCacheLoopTime;
// XTime totalFindTime;
// XTime totalAllocateTime;
// XTime totalLruTime;
// int OSSD_TICK_PER_SEC = 500000000;

int wchunk_select_chunk(MapSegmentCache *ccache, unsigned int logicalSliceAddr,
                        int isAllocate) {
    XTime startTime, cacheLoopTime, findTime, allocateTime, lruTime;
    int selectedSlot, bypassAlexFind = 0;
    int tree_num;
    FunctionalMappingTree *fmTree;
    MapSegment_p selectedChunk = NULL;
    // alex::Alex<unsigned int, WChunk_p>::Iterator it;

    unsigned int matchingChunkStartAddr =
        logicalSliceAddr & MAPSEG_START_ADDR_MASK;

#if WCHUNK_USE_LAST_SLOT
    // select chunk
    if (ccache->lastSelectedSlot >= 0 &&
        ccache->wchunkStartAddr[ccache->lastSelectedSlot] ==
            matchingChunkStartAddr) {
        selectedSlot = ccache->lastSelectedSlot;
        selectedChunk = ccache->wchunk_p[ccache->lastSelectedSlot];
        goto found;
    }
#endif

    // XTime_GetTime(&startTime);
    for (int i = 0; i < ccache->curItemCount; i++) {
        unsigned int chunkStartAddr = ccache->mapSegmentStartAddr[i];
        // check if hit
        if (chunkStartAddr == matchingChunkStartAddr) {
            selectedChunk = ccache->mapSegment_p[i];
            selectedSlot = i;
            goto found;
        }
    }
    // XTime_GetTime(&cacheLoopTime);

    // bypass find
    // because alex loops when no element is inserted
    if (ccache->curItemCount == 0) bypassAlexFind = 1;

    // find from tree
    // if (!bypassAlexFind) it = wchunktree.find(matchingChunkStartAddr);
    // XTime_GetTime(&findTime);

    // xil_printf("here3\n");
    // if (bypassAlexFind || it.cur_leaf_ == nullptr || !it.payload()) {
    //     if (!isAllocate) return -1;

    //     // allocate new chunk
    //     selectedChunk = wchunk_allocate_new(ccache, matchingChunkStartAddr);
    // } else {
    //     selectedChunk = it.payload();
    // }
    // XTime_GetTime(&allocateTime);

    tree_num = LSA_TO_TREE_NUM(logicalSliceAddr);
//    xil_printf("lsa: %p, tree_num: %d\n", logicalSliceAddr, tree_num);
    fmTree = &fmTrees[LSA_TO_TREE_NUM(logicalSliceAddr)];
    if (isFmTreesInitialized[LSA_TO_TREE_NUM(logicalSliceAddr)] == 0) {
        isFmTreesInitialized[LSA_TO_TREE_NUM(logicalSliceAddr)] = 1;
        xil_printf("FmTree %d first access: %p\n", LSA_TO_TREE_NUM(logicalSliceAddr), logicalSliceAddr);
    }
    selectedChunk = fetchChunkFromFmTree(fmTree, logicalSliceAddr, isAllocate);
    if (selectedChunk == NULL) return -1;

    // if chunk is found, find a slot
    if (ccache->curItemCount < MAPSEG_CACHE_SIZE) {
        selectedSlot = ccache->curItemCount;
        ccache->curItemCount++;
    } else {
        selectedSlot = wchunk_get_lru_slot(ccache);
    }

    if (selectedSlot < 0) {
        xil_printf("slot error lsa=%p\n", logicalSliceAddr);
        assert(!"slot not exist!");
    }

    // assign to the slot
    ccache->mapSegmentStartAddr[selectedSlot] = matchingChunkStartAddr;
    ccache->mapSegment_p[selectedSlot] = selectedChunk;
    // XTime_GetTime(&lruTime);

    // totalCacheLoopTime += (cacheLoopTime - startTime);
    // totalFindTime += (findTime - cacheLoopTime);
    // totalAllocateTime += (allocateTime - findTime);
    // totalLruTime += (lruTime - allocateTime);
    // calls++;

    // if (1.0 * (startTime - lastReportTime) / (OSSD_TICK_PER_SEC) >= 10) {
    // 	char reportString[1024];
    // 	sprintf(reportString,
    // 	"sec %f reporting calls: %d avg_cTime: %f avg_fTime: %f avg_aTime: %f
    // avg_lTime: %f\n", 		1.0 * startTime / (OSSD_TICK_PER_SEC),
    // calls, 		1.0
    // * totalCacheLoopTime / OSSD_TICK_PER_SEC * 1000000 /
    // calls, 		1.0 * totalFindTime / OSSD_TICK_PER_SEC * 1000000 /
    // calls, 		1.0 * totalAllocateTime / OSSD_TICK_PER_SEC * 1000000 /
    // calls, 		1.0 * totalLruTime / OSSD_TICK_PER_SEC * 1000000 /
    // calls); 	xil_printf("%s", reportString);

    // 	lastReportTime = startTime;
    // 	calls = 0;
    // 	totalCacheLoopTime = 0;
    // 	totalFindTime = 0;
    //     totalAllocateTime = 0;
    //     totalLruTime = 0;
    // }
found:
    // mark as most recently used one
    wchunk_mark_mru(ccache, selectedSlot);

#if WCHUNK_USE_LAST_SLOT
    // mark as last selected
    ccache->lastSelectedSlot = selectedSlot;
#endif

    return selectedSlot;
}

unsigned int mapseg_get(MapSegmentBucket *wchunkBucket,
                        unsigned int logicalSliceAddr) {
    unsigned int virtualSliceAddr, selectedChunkStartAddr, indexInChunk;
    MapSegmentCache *ccache;
    MapSegment_p selectedChunk;

//    xil_printf("wchunk start get %p\n", logicalSliceAddr);
    ccache = &wchunkBucket->mapSegmentCaches[MAPSEG_BUCKET_INDEX(logicalSliceAddr)];

    // directly return VSA_NONE on item count is zero
    // because alex loops when no element is inserted
    if (ccache->curItemCount == 0) {
//        xil_printf("wchunk fail get %p, %p\n", logicalSliceAddr, VSA_NONE);
        return VSA_NONE;
    }

    int selectedSlot = wchunk_select_chunk(ccache, logicalSliceAddr, 0);
    if (selectedSlot < 0) {
        return VSA_FAIL;
    }
    selectedChunk = ccache->mapSegment_p[selectedSlot];
    selectedChunkStartAddr = ccache->mapSegmentStartAddr[selectedSlot];
    indexInChunk = logicalSliceAddr - selectedChunkStartAddr;

    if (!mapseg_is_valid(ccache, selectedChunk, indexInChunk)) return VSA_FAIL;

    virtualSliceAddr = selectedChunk->entries[indexInChunk].virtualSliceAddr;

//    xil_printf("wchunk end get %p, %p\n", logicalSliceAddr, virtualSliceAddr);
    return virtualSliceAddr;
}
// XTime lastReportTime_I = 0;
// int calls = 0;
// XTime totalSelectTime = 0;
// XTime totalMidTime = 0;
// XTime totalMarkTime = 0;
// XTime maxSelectTime = 0;
// XTime maxMidTime = 0;
// XTime maxMarkTime = 0;
// int OSSD_TICK_PER_SEC = 500000000;

int mapseg_set(MapSegmentBucket *wchunkBucket, unsigned int logicalSliceAddr,
               unsigned int virtualSliceAddr) {
    // XTime startTime, selectTime, midTime, markTime;
    unsigned int selectedChunkStartAddr, indexInChunk;
    MapSegmentCache *ccache;
    MapSegment_p selectedChunk;
//    xil_printf("wchunk start set %p, %p\n", logicalSliceAddr, virtualSliceAddr);


    // if (virtualSliceAddr == VSA_NONE)
    // XTime_GetTime(&startTime);
    ccache = &wchunkBucket->mapSegmentCaches[MAPSEG_BUCKET_INDEX(logicalSliceAddr)];

    int selectedSlot = wchunk_select_chunk(ccache, logicalSliceAddr, 1);
    if (selectedSlot < 0) {
//        xil_printf("wchunk fail set %p, %p\n", logicalSliceAddr, virtualSliceAddr);
        return -1;
    }
    // if (virtualSliceAddr == VSA_NONE)
    // XTime_GetTime(&selectTime);
    selectedChunk = ccache->mapSegment_p[selectedSlot];
    selectedChunkStartAddr = ccache->mapSegmentStartAddr[selectedSlot];
    indexInChunk = logicalSliceAddr - selectedChunkStartAddr;

    selectedChunk->entries[indexInChunk].virtualSliceAddr = virtualSliceAddr;
    // if (virtualSliceAddr == VSA_NONE)
    // XTime_GetTime(&midTime);

    if (virtualSliceAddr != VSA_NONE)
        mapseg_mark_valid(ccache, selectedChunk, indexInChunk, 1,
                          selectedChunkStartAddr, 1, MAPSEG_FULL_BITS_IN_SLICE);
    else
        mapseg_mark_valid(ccache, selectedChunk, indexInChunk, 1,
                          selectedChunkStartAddr, 0, MAPSEG_FULL_BITS_IN_SLICE);

    // if (virtualSliceAddr == VSA_NONE)
    // XTime_GetTime(&markTime);

    // if (virtualSliceAddr == VSA_NONE)
    // calls++;

    // if (virtualSliceAddr == VSA_NONE)
    // totalSelectTime += (selectTime - startTime);
    // if (virtualSliceAddr == VSA_NONE)
    // totalMidTime += (midTime - selectTime);
    // if (virtualSliceAddr == VSA_NONE)
    // totalMarkTime += (markTime - midTime);

    // if (virtualSliceAddr == VSA_NONE)
    // if (maxSelectTime < selectTime - startTime)
    // 	maxSelectTime = selectTime - startTime;
    // if (virtualSliceAddr == VSA_NONE)
    // if (maxMidTime < midTime - selectTime)
    // 	maxMidTime = midTime - selectTime;
    // if (virtualSliceAddr == VSA_NONE)
    // if (maxMarkTime < markTime - midTime)
    // 	maxMarkTime = markTime - midTime;

    // if (virtualSliceAddr == VSA_NONE)
    // if (1.0 * (startTime - lastReportTime_I) / (OSSD_TICK_PER_SEC) >= 10) {
    // 	char reportString[1024];
    // 	sprintf(reportString,
    // 	"sec %f reporting calls: %d avg_selectTime: %f avg_midTime: %f
    // avg_markTime: %f max_selectTime: %f max_midTime: %f max_markTime: %f
    // %f\n", 		1.0 * startTime / (OSSD_TICK_PER_SEC),
    // calls, 		1.0
    // * totalSelectTime / OSSD_TICK_PER_SEC * 1000000 /
    // calls, 		1.0 * totalMidTime / OSSD_TICK_PER_SEC * 1000000 /
    // calls, 		1.0 * totalMarkTime / OSSD_TICK_PER_SEC * 1000000 /
    // calls, 		1.0 * maxSelectTime / OSSD_TICK_PER_SEC * 1000000,
    // 1.0 * maxMidTime / OSSD_TICK_PER_SEC *
    // 1000000, 		1.0 * maxMarkTime / OSSD_TICK_PER_SEC *
    // 1000000); 	xil_printf("%s", reportString);

    // 	lastReportTime_I = startTime;
    // 	calls = 0;
    // 	totalSelectTime = 0;
    // 	totalMidTime = 0;
    // 	totalMarkTime = 0;
    // 	maxSelectTime = 0;
    // 	maxMidTime = 0;
    // 	maxMarkTime = 0;
    // }
//    xil_printf("wchunk end set %p, %p\n", logicalSliceAddr, virtualSliceAddr);
    return 0;
}

int mapseg_remove(MapSegmentBucket *wchunkBucket, unsigned int logicalSliceAddr) {
    return mapseg_set(wchunkBucket, logicalSliceAddr, VSA_NONE);
}

void mapseg_deallocate(MapSegmentCache *ccache, MapSegment_p wchunk_p,
                       unsigned int chunkStartAddr) {
    alex::Alex<unsigned int, MapSegment_p>::Iterator it;

    // TODO: implement fm deallocate
    //    it = wchunktree.find(chunkStartAddr);
    //    if (it.cur_leaf_ != nullptr) it.payload() = 0;

    // xil_printf("wchunk deallocating chunk@%p, with startAddr=%p\n", wchunk_p,
    //            chunkStartAddr);

    // swap with the last one, and decrement the item count
    for (int i = 0; i < ccache->curItemCount; i++) {
        unsigned int startAddr = ccache->mapSegmentStartAddr[i];
        // check if hit
        if (startAddr == chunkStartAddr) {
            // migrate the last slot's one and decrement
            ccache->mapSegmentStartAddr[i] =
                ccache->mapSegmentStartAddr[ccache->curItemCount - 1];
            ccache->mapSegment_p[i] = ccache->mapSegment_p[ccache->curItemCount - 1];
            ccache->lruValues[i] = ccache->lruValues[ccache->curItemCount - 1];
            ccache->curItemCount--;
        }
    }
}

int wchunk_get_lru_slot(MapSegmentCache *ccache) {
    int minLruVal, minLruSlot;

    // if cache is not full, no eviction
    if (ccache->curItemCount < MAPSEG_CACHE_SIZE) return -1;

    // set to first item
    minLruVal = ccache->lruValues[0];
    minLruSlot = 0;

    // evict from lru and put into WChunkTree and return slot
    for (int i = 1; i < ccache->curItemCount; i++) {
        if (minLruVal > ccache->lruValues[i]) {
            minLruVal = ccache->lruValues[i];
            minLruSlot = i;
        }
    }

    return minLruSlot;
}

// TODO: handle overflow or take looping approach (measure performance)
void wchunk_mark_mru(MapSegmentCache *ccache, int slot) {
    ccache->lruValues[slot] = ccache->maxLruValue;
    ccache->maxLruValue++;
}

int mapseg_is_valid(MapSegmentCache *ccache, MapSegment_p wchunk_p,
                    unsigned int indexInChunk) {
    int validBitIndex, validBitSelector;
    validBitIndex = MAPSEG_VALID_BIT_INDEX(indexInChunk);
    validBitSelector =
        MAPSEG_VALID_BIT_SELECTOR(indexInChunk, MAPSEG_FULL_BITS_IN_SLICE);

    return wchunk_p->validBits[validBitIndex] & validBitSelector;
}

void mapseg_mark_valid(MapSegmentCache *ccache, MapSegment_p wchunk_p,
                       unsigned int indexInChunk, int length,
                       unsigned int wchunkStartAddr, int isValid,
                       int bitsInSlice) {
    int validBitIndex, validBitSelector, origBits, newBits, numBits = 0;
    for (int i = 0; i < length; i++) {
        validBitIndex = MAPSEG_VALID_BIT_INDEX(indexInChunk + i);
        validBitSelector =
            MAPSEG_VALID_BIT_SELECTOR(indexInChunk + i, bitsInSlice);

        origBits = wchunk_p->validBits[validBitIndex];

        if (isValid) {
            // if original value was already on, just return
            // if (origBits & validBitSelector == validBitSelector) return;
            newBits = origBits | validBitSelector;
        } else {
            // if original value was already off, just return
            // if (!(origBits & validBitSelector)) return;
            newBits = origBits & (~validBitSelector);

            // if (length == 1)
            //     xil_printf(
            //         "mark_invalid: startAddr=%d, selector=%p, newpop=%d, "
            //         "oldpop=%d\n",
            //         wchunkStartAddr, validBitSelector,
            //         __builtin_popcountl(newBits),
            //         __builtin_popcountl(origBits));

            // if (wchunk_p->numOfValidBits < 100) {
            //     xil_printf("found low chunk: startAddr=%p, num=%d\n",
            //                wchunkStartAddr, wchunk_p->numOfValidBits);
            //            }
        }
        wchunk_p->numOfValidMaps +=
            __builtin_popcountl(newBits) - __builtin_popcountl(origBits);
        wchunk_p->validBits[validBitIndex] = newBits;
    }

    // if (!isValid && wchunk_p->numOfValidBits % 1000 == 0) {
    //     xil_printf(
    //         "wchunk marking invalid: startAddr=%p, index=%d, bitIndex=%d, "
    //         "selector=%p, lastBits=%d\n",
    //         wchunkStartAddr, indexInChunk, validBitIndex, validBitSelector,
    //         wchunk_p->numOfValidBits);
    // }

    // deallocate totally unused chunk
    if (wchunk_p->numOfValidMaps == 0)
        mapseg_deallocate(ccache, wchunk_p, wchunkStartAddr);
    // mapseg_add_erase_chunk(wchunk_p, wchunkStartAddr);
}

int mapseg_mark_valid_partial(MapSegmentBucket *wchunkBucket,
                              unsigned int logicalSliceAddr, int isValid,
                              int start, int end) {
    unsigned int selectedChunkStartAddr, indexInChunk;
    MapSegmentCache *ccache;
    MapSegment_p selectedChunk;

    int validBitIndex, bitsInSlice, origBits, newBits;

    ccache = &wchunkBucket->mapSegmentCaches[MAPSEG_BUCKET_INDEX(logicalSliceAddr)];

    // directly return VSA_NONE on item count is zero
    // because alex loops when no element is inserted
    if (ccache->curItemCount == 0) {
        return 0;
    }

    int selectedSlot = wchunk_select_chunk(ccache, logicalSliceAddr, 0);
    if (selectedSlot < 0) {
        return 0;
    }
    selectedChunk = ccache->mapSegment_p[selectedSlot];
    selectedChunkStartAddr = ccache->mapSegmentStartAddr[selectedSlot];
    indexInChunk = logicalSliceAddr - selectedChunkStartAddr;

    bitsInSlice = 0;
    for (int i = start; i < end; i++) {
        bitsInSlice += (1 << (3 - i));
    }
    // xil_printf(
    //     "before partial mark: addr=%d, validNum=%d, start=%d, end=%d\n",
    //     logicalSliceAddr, selectedChunk->numOfValidBits, start, end);

    mapseg_mark_valid(ccache, selectedChunk, indexInChunk, 1,
                      selectedChunkStartAddr, isValid, bitsInSlice);

    // return 1 if the entry is totally invalidated, else 0
    return !mapseg_is_valid(ccache, selectedChunk, indexInChunk);

    // xil_printf("partial mark: startAddr=%d, validNum=%d\n",
    //            selectedChunkStartAddr, selectedChunk->numOfValidBits);
}

void mapseg_add_erase_chunk(MapSegment_p wchunk_p, unsigned int wchunkStartAddr) {
    wchunkEraseList.mapSegment_p[wchunkEraseList.curItemCount] = wchunk_p;
    wchunkEraseList.mapSegmentStartAddr[wchunkEraseList.curItemCount] =
        wchunkStartAddr;
    wchunkEraseList.curItemCount++;
}

void mapseg_handle_erase(MapSegmentBucket *wchunkBucket) {
    MapSegmentCache *ccache;
    for (int i = 0; i < wchunkEraseList.curItemCount; i++) {
        ccache = &wchunkBucket->mapSegmentCaches[MAPSEG_BUCKET_INDEX(
            wchunkEraseList.mapSegmentStartAddr[i])];
        mapseg_deallocate(ccache, wchunkEraseList.mapSegment_p[i],
                          wchunkEraseList.mapSegmentStartAddr[i]);
    }
    wchunkEraseList.curItemCount = 0;
}
