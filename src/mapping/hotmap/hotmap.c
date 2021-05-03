/*
 * hotmap.c
 *
 *  Created on: 2021. 4. 28.
 *      Author: noble
 */

#include "hotmap.h"

#include "../../address_translation.h"

void hotmap_insert(alex::Alex<unsigned int, unsigned int> hotmap,
                   unsigned int logicalSliceAddr,
                   unsigned int virtualSliceAddr) {
    hotmap.insert(logicalSliceAddr, virtualSliceAddr);
}

unsigned int hotmap_find(alex::Alex<unsigned int, unsigned int> hotmap,
                         unsigned int logicalSliceAddr) {
    alex::Alex<unsigned int, unsigned int>::Iterator it;

    it = hotmap.find(logicalSliceAddr);
    if (it.cur_leaf_ == nullptr) return VSA_NONE;

    return it.payload();
}

unsigned int hotmap_erase(alex::Alex<unsigned int, unsigned int> hotmap,
                          unsigned int logicalSliceAddr) {
    hotmap.erase(logicalSliceAddr);
}