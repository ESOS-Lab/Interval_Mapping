/*
 * hotmap.c
 *
 *  Created on: 2021. 4. 28.
 *      Author: noble
 */

#include "../hotmap/hotmap_alex.h"

#include <xil_printf.h>

#include "../../address_translation.h"

void hotmap_insert(alex::Alex<unsigned int, unsigned int> &hotmap,
                   unsigned int logicalSliceAddr,
                   unsigned int virtualSliceAddr) {
    if (DEBUG_HOTMAP)
        xil_printf("hotmap_insert %p, %p\n", logicalSliceAddr,
                   virtualSliceAddr);
    hotmap.insert(logicalSliceAddr, virtualSliceAddr);
}

unsigned int hotmap_find(alex::Alex<unsigned int, unsigned int> &hotmap,
                         unsigned int logicalSliceAddr) {
    if (DEBUG_HOTMAP) xil_printf("hotmap_find %p\n", logicalSliceAddr);

    alex::Alex<unsigned int, unsigned int>::Iterator it;

    it = hotmap.find(logicalSliceAddr);
    if (it.cur_leaf_ == nullptr) return VSA_NONE;

    return it.payload();
}

void hotmap_erase(alex::Alex<unsigned int, unsigned int> &hotmap,
                  unsigned int logicalSliceAddr) {
    if (DEBUG_HOTMAP) xil_printf("hotmap_erase %p\n", logicalSliceAddr);
    hotmap.erase(logicalSliceAddr);
}

void hotmap_update(alex::Alex<unsigned int, unsigned int> &hotmap,
                   unsigned int logicalSliceAddr,
                   unsigned int virtualSliceAddr) {
    if (DEBUG_HOTMAP)
        xil_printf("hotmap_update %p, %p\n", logicalSliceAddr,
                   virtualSliceAddr);
    alex::Alex<unsigned int, unsigned int>::Iterator it;

    it = hotmap.find(logicalSliceAddr);
    if (it.cur_leaf_ == nullptr)
        assert(!"trying to update not assigned value in hotmap");
    it.payload() = virtualSliceAddr;
}