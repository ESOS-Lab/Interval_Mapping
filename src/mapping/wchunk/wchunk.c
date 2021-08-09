/*
 * wchunk.c
 *
 *  Created on: 2021. 7. 16.
 *      Author: Minsu Jang (nobleminsu@gmail.com)
 */

#include "wchunk.h"

#include <assert.h>
#include <string.h>

#include "../../alex/openssd_allocator.h"
#include "../../ftl_config.h"
#include "../../memory_map.h"
#include "xil_printf.h"

char *ftableMemPool = (char *)RESERVED0_START_ADDR;

// WChunkCache *ccache;
WChunkBucket *wchunkBucket;
WChunkTree wchunktree;
OpenSSDAllocator<WChunk> allocator = OpenSSDAllocator<WChunk>();

int wchunk_get_lru_slot(WChunkCache *ccache);
void wchunk_mark_mru(WChunkCache *ccache, int slot);
int wchunk_select_chunk(WChunkCache *ccache, unsigned int logicalSliceAddr,
                        int isAllocate);
WChunk_p wchunk_allocate_new(WChunkCache *ccache, unsigned int chunkStartAddr);
void wchunk_print_alex_stats();

void wchunk_init() {
    WChunkCache *ccache;

    OpenSSDAllocator<WChunkBucket> aa;
    wchunkBucket = (WChunkBucket_p)aa.allocate(1);

    for (int i = 0; i < WCHUNK_BUCKET_SIZE; i++) {
        ccache = &wchunkBucket->ccaches[i];
        ccache->curItemCount = 0;
        ccache->maxLruValue = 0;

#if WCHUNK_USE_LAST_SLOT
        ccache->lastSelectedSlot = -1;
#endif
    }
}

int wchunk_select_chunk(WChunkCache *ccache, unsigned int logicalSliceAddr,
                        int isAllocate) {
    int selectedSlot, bypassAlexFind = 0;
    WChunk_p selectedChunk = NULL;
    alex::Alex<unsigned int, WChunk_p>::Iterator it;

    unsigned int matchingChunkStartAddr =
        logicalSliceAddr & WCHUNK_START_ADDR_MASK;

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

    for (int i = 0; i < ccache->curItemCount; i++) {
        unsigned int chunkStartAddr = ccache->wchunkStartAddr[i];
        // check if hit
        if (chunkStartAddr == matchingChunkStartAddr) {
            selectedChunk = ccache->wchunk_p[i];
            selectedSlot = i;
            goto found;
        }
    }

    // bypass find
    // because alex loops when no element is inserted
    if (ccache->curItemCount == 0) bypassAlexFind = 1;

    // find from tree
    if (!bypassAlexFind) it = wchunktree.find(matchingChunkStartAddr);
    // xil_printf("here3\n");
    if (bypassAlexFind || it.cur_leaf_ == nullptr) {
        if (!isAllocate) return -1;

        // allocate new chunk
        selectedChunk = wchunk_allocate_new(ccache, matchingChunkStartAddr);
    } else {
        selectedChunk = it.payload();
    }

    // if chunk is found, find a slot
    if (ccache->curItemCount < WCHUNK_CACHE_SIZE) {
        selectedSlot = ccache->curItemCount;
        ccache->curItemCount++;
    } else {
        selectedSlot = wchunk_get_lru_slot(ccache);
    }

    if (selectedSlot < 0) assert(!"slot not exist!");

    // assign to the slot
    ccache->wchunkStartAddr[selectedSlot] = matchingChunkStartAddr;
    ccache->wchunk_p[selectedSlot] = selectedChunk;

found:
    // mark as most recently used one
    wchunk_mark_mru(ccache, selectedSlot);

#if WCHUNK_USE_LAST_SLOT
    // mark as last selected
    ccache->lastSelectedSlot = selectedSlot;
#endif

    return selectedSlot;
}

unsigned int wchunk_get(WChunkBucket *wchunkBucket,
                        unsigned int logicalSliceAddr) {
    unsigned int virtualSliceAddr, selectedChunkStartAddr;
    WChunkCache *ccache =
        &wchunkBucket->ccaches[WCHUNK_BUCKET_INDEX(logicalSliceAddr)];

    // directly return VSA_NONE on item count is zero
    // because alex loops when no element is inserted
    if (ccache->curItemCount == 0) {
        return VSA_NONE;
    }

    int selectedSlot = wchunk_select_chunk(ccache, logicalSliceAddr, 0);
    if (selectedSlot < 0) {
        return VSA_FAIL;
    }
    WChunk_p selectedChunk = ccache->wchunk_p[selectedSlot];
    selectedChunkStartAddr = ccache->wchunkStartAddr[selectedSlot];

    virtualSliceAddr =
        selectedChunk->entries[logicalSliceAddr - selectedChunkStartAddr]
            .virtualSliceAddr;

    return virtualSliceAddr;
}

int wchunk_set(WChunkBucket *wchunkBucket, unsigned int logicalSliceAddr,
               unsigned int virtualSliceAddr) {
    unsigned int selectedChunkStartAddr;
    WChunkCache *ccache =
        &wchunkBucket->ccaches[WCHUNK_BUCKET_INDEX(logicalSliceAddr)];

    int selectedSlot = wchunk_select_chunk(ccache, logicalSliceAddr, 1);
    if (selectedSlot < 0) {
        return -1;
    }
    WChunk_p selectedChunk = ccache->wchunk_p[selectedSlot];
    selectedChunkStartAddr = ccache->wchunkStartAddr[selectedSlot];

    selectedChunk->entries[logicalSliceAddr - selectedChunkStartAddr]
        .virtualSliceAddr = virtualSliceAddr;

    return 0;
}

int wchunk_remove(WChunkBucket *wchunkBucket, unsigned int logicalSliceAddr) {
    return wchunk_set(wchunkBucket, logicalSliceAddr, VSA_NONE);
}

WChunk_p wchunk_allocate_new(WChunkCache *ccache, unsigned int chunkStartAddr) {
    // WChunk_p chunkp = (WChunk_p)ftableMemPool;
    WChunk_p chunkp = (WChunk_p)allocator.allocate(1);
    // ftableMemPool += sizeof(WChunk);

    memset(chunkp, VSA_NONE, sizeof(WChunk));

    // for (int i = 0; i < WCHUNK_LENGTH; i++) {
    //     if (chunkp->entries[i].virtualSliceAddr != VSA_NONE)
    //         xil_printf("wchunk allocate error %d\n", i);
    // }

    wchunktree.insert(chunkStartAddr, chunkp);

    wchunk_print_alex_stats();

    size_t total, user, free;
	int nr_blocks;
    sm_malloc_stats(&total, &user, &free, &nr_blocks);
    xil_printf("cur memory state: total=%d, user=%d, free=%d, nr_blocks=%d\n", total, user, free, nr_blocks);
    
    return chunkp;
}



int wchunk_get_lru_slot(WChunkCache *ccache) {
    int minLruVal, minLruSlot;

    // if cache is not full, no eviction
    if (ccache->curItemCount < WCHUNK_CACHE_SIZE) return -1;

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
void wchunk_mark_mru(WChunkCache *ccache, int slot) {
    ccache->lruValues[slot] = ccache->maxLruValue;
    ccache->maxLruValue++;
}

void wchunk_print_alex_stats() {
    WChunkTree::Stats stats = wchunktree.get_stats();
    xil_printf(
        "num_keys=%d, num_model_nodes=%d, num_data_nodes=%d, num_splits=%d\n",
        stats.num_keys, stats.num_model_nodes, stats.num_data_nodes,
        stats.num_downward_splits + stats.num_sideways_splits);
    wchunktree.report_models();
}
