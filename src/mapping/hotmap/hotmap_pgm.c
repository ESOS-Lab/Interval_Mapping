/*
 * hotmap.c
 *
 *  Created on: 2021. 4. 28.
 *      Author: noble
 */

#include "../hotmap/hotmap_pgm.h"

#include <xil_printf.h>

#include "../../address_translation.h"

void hotmap_pgm_insert(pgm::DynamicPGMIndex<unsigned int, unsigned int> &hotmap,
                       unsigned int logicalSliceAddr,
                       unsigned int virtualSliceAddr) {
    if (DEBUG_HOTMAP)
        xil_printf("hotmap_insert %p, %p\n", logicalSliceAddr,
                   virtualSliceAddr);
    hotmap.insert_or_assign(logicalSliceAddr, virtualSliceAddr);
}

unsigned int hotmap_pgm_find(
    pgm::DynamicPGMIndex<unsigned int, unsigned int> &hotmap,
    unsigned int logicalSliceAddr) {
    if (DEBUG_HOTMAP) xil_printf("hotmap_find %p\n", logicalSliceAddr);

    //    pgm::DynamicPGMIndex<unsigned int, unsigned int>::Iterator it;

    auto it = hotmap.find(logicalSliceAddr);
    if (it == hotmap.end()) return VSA_NONE;

    return it->second;
}

void hotmap_pgm_erase(pgm::DynamicPGMIndex<unsigned int, unsigned int> &hotmap,
                      unsigned int logicalSliceAddr) {
    if (DEBUG_HOTMAP) xil_printf("hotmap_erase %p\n", logicalSliceAddr);
    hotmap.erase(logicalSliceAddr);
}

void hotmap_pgm_update(pgm::DynamicPGMIndex<unsigned int, unsigned int> &hotmap,
                       unsigned int logicalSliceAddr,
                       unsigned int virtualSliceAddr) {
    if (DEBUG_HOTMAP)
        xil_printf("hotmap_update %p, %p\n", logicalSliceAddr,
                   virtualSliceAddr);

    hotmap.insert_or_assign(logicalSliceAddr, virtualSliceAddr);
}
