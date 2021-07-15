/*
 * ftable.c
 *
 *  Created on: 2021. 4. 26.
 *      Author: Minsu Jang (nobleminsu@gmail.com)
 */

#include "ftable.h"

#include <assert.h>
#include <string.h>

#include "../../alex/openssd_allocator.h"
#include "../../ftl_config.h"
#include "../../memory_map.h"
#include "xil_printf.h"

char *ftableMemPool = (char *)RESERVED0_START_ADDR;

WChunkCache *ccache;
WChunkTree wchunktree;
OpenSSDAllocator<WChunk> allocator = OpenSSDAllocator<WChunk>();

int wchunk_get_lru_slot(WChunkCache *ccache);
void wchunk_mark_mru(WChunkCache *ccache, int slot);
int wchunk_select_chunk(WChunkCache *ccache, unsigned int logicalSliceAddr,
                        int isAllocate);
WChunk_p wchunk_allocate_new(WChunkCache *ccache, unsigned int chunkStartAddr);

void wchunk_init() {
    OpenSSDAllocator<WChunkCache> aa;
    ccache = (WChunkCache *)aa.allocate(1);
    ccache->curItemCount = 0;
    ccache->maxLruValue = 0;
    ccache->lastSelectedSlot = -1;
}

int wchunk_select_chunk(WChunkCache *ccache, unsigned int logicalSliceAddr,
                        int isAllocate) {
    int selectedSlot, slot, bypassAlexFind = 0;
    WChunk_p selectedChunk = NULL;
    alex::Alex<unsigned int, WChunk_p>::Iterator it;

    unsigned int matchingChunkStartAddr =
        logicalSliceAddr & WCHUNK_CHUNK_SIZE_MASK;

    // select chunk
    if (ccache->lastSelectedSlot >= 0 &&
        ccache->wchunkStartAddr[ccache->lastSelectedSlot] ==
            matchingChunkStartAddr) {
        selectedSlot = ccache->lastSelectedSlot;
        selectedChunk = ccache->wchunk_p[ccache->lastSelectedSlot];
        goto found;
    }

    for (int i = 0; i < ccache->curItemCount; i++) {
        unsigned int chunkStartAddr = ccache->wchunkStartAddr[i];
        // check if hit
        if (chunkStartAddr == matchingChunkStartAddr) {
            selectedChunk = ccache->wchunk_p[i];
            selectedSlot = i;
            goto found;
        }
    }

    // if not found
    // evict lru one

    // bypass find
    // because alex loops when no element is inserted
    if (ccache->curItemCount == 0) bypassAlexFind = 1;

    if (ccache->curItemCount < WCHUNK_CACHE_SIZE)
        slot = ccache->curItemCount++;
    else {
        slot = wchunk_get_lru_slot(ccache);
    }

    if (slot < 0) assert(!"slot not exist!");

    // find from tree
    if (!bypassAlexFind) it = wchunktree.find(matchingChunkStartAddr);
    // xil_printf("here3\n");
    if (bypassAlexFind || it.cur_leaf_ == nullptr) {
        if (!isAllocate) return -1;

        // allocate new chunk
        selectedChunk = wchunk_allocate_new(ccache, matchingChunkStartAddr);
        // ccache->isNewlyAllocated[slot] = 1;
    } else {
        selectedChunk = it.payload();
        // ccache->isNewlyAllocated[slot] = 0;
    }

    ccache->wchunkStartAddr[slot] = matchingChunkStartAddr;
    ccache->wchunk_p[slot] = selectedChunk;

    selectedSlot = slot;

found:
    // mark as most recently used one
    wchunk_mark_mru(ccache, selectedSlot);

    // mark as last selected
    ccache->lastSelectedSlot = selectedSlot;

    return selectedSlot;
}

unsigned int wchunk_get(WChunkCache *ccache, unsigned int logicalSliceAddr) {
    unsigned int virtualSliceAddr, selectedChunkStartAddr;

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

int wchunk_set(WChunkCache *ccache, unsigned int logicalSliceAddr,
               unsigned int virtualSliceAddr) {
    unsigned int selectedChunkStartAddr;

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

int wchunk_remove(WChunkCache *ccache, unsigned int logicalSliceAddr) {
    return wchunk_set(ccache, logicalSliceAddr, VSA_NONE);
}

WChunk_p wchunk_allocate_new(WChunkCache *ccache, unsigned int chunkStartAddr) {
    // WChunk_p chunkp = (WChunk_p)ftableMemPool;
    WChunk_p chunkp = (WChunk_p)allocator.allocate(1);
    // ftableMemPool += sizeof(WChunk);

    memset(chunkp, VSA_NONE, sizeof(WChunk));

    for (int i = 0; i < FTABLE_DEFAULT_CHUNK_SIZE; i++) {
        if (chunkp->entries[i].virtualSliceAddr != VSA_NONE)
            xil_printf("wchunk allocate error %d\n", i);
    }

    wchunktree.insert(chunkStartAddr, chunkp);

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
