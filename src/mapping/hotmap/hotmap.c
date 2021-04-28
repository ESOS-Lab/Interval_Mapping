/*
 * hotmap.c
 *
 *  Created on: 2021. 4. 28.
 *      Author: noble
 */

#include "hotmap.h"

#include "../../address_translation.h"

alex::Alex<unsigned int, unsigned int> hotmap;

void hotmap_insert(unsigned int logicalSliceAddr,
                   unsigned int virtualSliceAddr) {
    hotmap.insert(logicalSliceAddr, virtualSliceAddr);
}

unsigned int hotmap_find(unsigned int logicalSliceAddr) {
    alex::Alex<unsigned int, unsigned int>::Iterator it;

    it = hotmap.find(logicalSliceAddr);
    if (it.cur_leaf_ == nullptr) return VSA_NONE;

    return it.payload();
}

unsigned int hotmap_erase(unsigned int logicalSliceAddr) {
    hotmap.erase(logicalSliceAddr);
}