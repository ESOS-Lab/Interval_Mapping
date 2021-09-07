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

#define WCHUNK_USE_LAST_SLOT 1

#define WCHUNK_LENGTH_DIGIT 14
#define WCHUNK_BUCKET_DIGIT 4
#define WCHUNK_PIECE_SIZE 64
#define WCHUNK_LENGTH (1 << WCHUNK_LENGTH_DIGIT)
#define WCHUNK_BUCKET_SIZE (1 << WCHUNK_BUCKET_DIGIT)
#define WCHUNK_CACHE_SIZE 20
#define WCHUNK_PIECE_COUNT_IN_CHUNK (WCHUNK_LENGTH / WCHUNK_PIECE_SIZE)
#define WCHUNK_START_ADDR_MASK (~(WCHUNK_LENGTH - 1))
#define WCHUNK_BUCKET_INDEX_MASK \
    ((WCHUNK_BUCKET_SIZE - 1) << WCHUNK_LENGTH_DIGIT)
#define WCHUNK_BUCKET_INDEX(lsa) \
    ((lsa & WCHUNK_BUCKET_INDEX_MASK) >> WCHUNK_LENGTH_DIGIT)
#define WCHUNK_PIECE_IDX(idx) (idx / WCHUNK_PIECE_SIZE)
#define WCHUNK_IDX_IN_PIECE(idx) (idx % WCHUNK_PIECE_SIZE)

#define WCHUNK_VALID_BIT_INDEX(index) (index >> 3)
#define WCHUNK_VALID_BIT_SELECTOR(index, bitsInSlice) \
    (bitsInSlice << (28 - ((index & 0x7) << 2)))
#define WCHUNK_FULL_BITS_IN_SLICE 0xF

#define WCHUNK_ERASE_LIST_LENGTH (1024)

typedef struct wchunk_piece {
    int numOfValidBits;
    unsigned int validBits[WCHUNK_VALID_BIT_INDEX(WCHUNK_PIECE_SIZE)];
    LOGICAL_SLICE_ENTRY entries[WCHUNK_PIECE_SIZE];
} WChunkPiece;

typedef struct wchunk {
    int numOfValidBits;
    WChunkPiece *pieces[WCHUNK_PIECE_COUNT_IN_CHUNK];
} WChunk, *WChunk_p;

using WChunkTree = alex::Alex<unsigned int, WChunk_p>;
extern OpenSSDAllocator<WChunkPiece> pieceAllocator;

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
                       unsigned int wchunkStartAddr, int isValid,
                       int bitsInSlice);
int wchunk_mark_valid_partial(WChunkBucket *wchunkBucket,
                              unsigned int logicalSliceAddr, int isValid,
                              int start, int end);

void wchunk_add_erase_chunk(WChunk_p wchunk_p, unsigned int wchunkStartAddr);
void wchunk_handle_erase(WChunkBucket *wchunkBucket);
extern WChunkBucket *wchunkBucket;

inline unsigned int wchunk_entry_from_index(WChunk_p wchunk_p,
                                            int indexInChunk) {
    WChunkPiece *piece = wchunk_p->pieces[WCHUNK_PIECE_IDX(indexInChunk)];
    int validBitIndex =
        WCHUNK_VALID_BIT_INDEX(WCHUNK_IDX_IN_PIECE(indexInChunk));
    if (piece == NULL)
        return VSA_NONE;
    return piece->entries[WCHUNK_IDX_IN_PIECE(indexInChunk)].virtualSliceAddr;
}

inline void wchunk_set_entry_from_index(WChunk_p wchunk_p, int indexInChunk,
                                        unsigned int virtualSliceAddr) {
    WChunkPiece *piece = wchunk_p->pieces[WCHUNK_PIECE_IDX(indexInChunk)];
    if (piece == NULL) return;
    piece->entries[WCHUNK_IDX_IN_PIECE(indexInChunk)].virtualSliceAddr =
        virtualSliceAddr;
}

inline WChunkPiece *wchunk_allocate_piece_whole_chunk(WChunk_p wchunk_p) {
    WChunkPiece *p = pieceAllocator.allocate(WCHUNK_PIECE_COUNT_IN_CHUNK);

    for (int i = 0; i < WCHUNK_PIECE_COUNT_IN_CHUNK; i++) {
        p[i].numOfValidBits = 0;
        memset(
            &p[i].validBits, 0,
            sizeof(unsigned int) * WCHUNK_VALID_BIT_INDEX(WCHUNK_PIECE_SIZE));
        memset(&p[i].entries, VSA_NONE,
               sizeof(LOGICAL_SLICE_ENTRY) * WCHUNK_PIECE_SIZE);
        wchunk_p->pieces[i] = &p[i];
    }

    return p;
}

inline int wchunk_get_valid_bits(WChunk_p wchunk_p, int indexInChunk) {
    WChunkPiece *piece = wchunk_p->pieces[WCHUNK_PIECE_IDX(indexInChunk)];
    if (piece == NULL) return 0;
    int validBitIndex =
        WCHUNK_VALID_BIT_INDEX(WCHUNK_IDX_IN_PIECE(indexInChunk));

    return piece->validBits[validBitIndex];
}

inline void wchunk_set_valid_bits(WChunk_p wchunk_p, int indexInChunk,
                                  int newBits) {
    WChunkPiece *piece = wchunk_p->pieces[WCHUNK_PIECE_IDX(indexInChunk)];
    if (piece == NULL) return;
    int validBitIndex =
        WCHUNK_VALID_BIT_INDEX(WCHUNK_IDX_IN_PIECE(indexInChunk));

    int origBits = piece->validBits[validBitIndex];

    piece->numOfValidBits +=
        __builtin_popcountl(newBits) - __builtin_popcountl(origBits);
    wchunk_p->numOfValidBits +=
        __builtin_popcountl(newBits) - __builtin_popcountl(origBits);
    piece->validBits[validBitIndex] = newBits;
}

#endif /* SRC_MAPPING_WCHUNK_WCHUNK_H_ */
