/*
 * coldmap.h
 *
 *  Created on: 2021. 4. 28.
 *      Author: noble
 */

#ifndef SRC_MAPPING_COLDMAP_COLDMAP_H_
#define SRC_MAPPING_COLDMAP_COLDMAP_H_

#include "../../address_translation.h"
#include "../../alex/alex.h"
#include "../../ftl_config.h"

// mask to align items per segment (in FTL term, BLOCK)
#define COLDMAP_ADDR_INDEX_MASK \
    (~(SLICES_PER_BLOCK - 1))  // mask out 127 (7 bits) now
#define COLDMAP_ADDR_OFFSET_MASK (SLICES_PER_BLOCK - 1)  // max 127 now

struct coldmap_entry {
    int activeCount = 0;
    unsigned int vsas[SLICES_PER_BLOCK];
};

extern alex::Alex<unsigned int, struct coldmap_entry> coldmap;

void coldmap_insert(unsigned int logicalSliceAddr,
                    unsigned int virtualSliceAddr);

unsigned int coldmap_find(unsigned int logicalSliceAddr);

unsigned int coldmap_erase(unsigned int logicalSliceAddr);

inline unsigned int coldmap_index(unsigned int logicalSliceAddr) {
    return ((unsigned int)COLDMAP_ADDR_INDEX_MASK) & logicalSliceAddr;
}

inline unsigned int coldmap_offset(unsigned int logicalSliceAddr) {
    return ((unsigned int)COLDMAP_ADDR_OFFSET_MASK) & logicalSliceAddr;
}

#endif /* SRC_MAPPING_COLDMAP_COLDMAP_H_ */
