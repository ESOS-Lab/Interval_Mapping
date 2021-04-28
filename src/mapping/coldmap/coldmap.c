/*
 * coldmap.c
 *
 *  Created on: 2021. 4. 28.
 *      Author: noble
 */

#include "coldmap.h"
#include <algorithm>


int coldmap_check_is_entry_clear(struct coldmap_entry *centry);

alex::Alex<unsigned int, struct coldmap_entry> coldmap;

void coldmap_insert(unsigned int logicalSliceAddr,
                    unsigned int virtualSliceAddr) {
    unsigned int cidx = coldmap_index(logicalSliceAddr);
    unsigned int cofs = coldmap_offset(logicalSliceAddr);
    alex::Alex<unsigned int, struct coldmap_entry>::Iterator it;

    it = coldmap.find(cidx);
    if (it.cur_leaf_ == nullptr) {
        // if entry is not found, insert one
        struct coldmap_entry centry;
        std::fill(centry.vsas, centry.vsas + SLICES_PER_BLOCK, VSA_NONE);
        centry.vsas[cofs] = virtualSliceAddr;
        centry.activeCount++;
        coldmap.insert(cidx, centry);
    } else {
        // if the entry exists, change the mapping
        it.payload().vsas[cofs] = virtualSliceAddr;
    }
}

unsigned int coldmap_find(unsigned int logicalSliceAddr) {
    unsigned int cidx = coldmap_index(logicalSliceAddr);
    unsigned int cofs = coldmap_offset(logicalSliceAddr);
    alex::Alex<unsigned int, struct coldmap_entry>::Iterator it;

    it = coldmap.find(cidx);
    if (it.cur_leaf_ == nullptr) return VSA_NONE;

    return it.payload().vsas[cofs];
}

unsigned int coldmap_erase(unsigned int logicalSliceAddr) {
    unsigned int cidx = coldmap_index(logicalSliceAddr);
    unsigned int cofs = coldmap_offset(logicalSliceAddr);
    alex::Alex<unsigned int, struct coldmap_entry>::Iterator it;

    it = coldmap.find(cidx);
    if (it.cur_leaf_ != nullptr) {
        // if coressponding segment node is found
        struct coldmap_entry *p_centry = &it.payload();
        if (p_centry->vsas[cofs] != VSA_NONE) {
            // if coressponding vsa is valid, erase the value
            p_centry->vsas[cofs] = VSA_NONE;
            p_centry->activeCount--;

            if (p_centry->activeCount == 0) {
                // if there is no active item in the entry, erase it from
                // coldmap
                coldmap.erase(cidx);
            }
        }
    }
}
