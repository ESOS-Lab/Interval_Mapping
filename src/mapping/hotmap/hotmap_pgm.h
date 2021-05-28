/*
 * hotmap.h
 *
 *  Created on: 2021. 4. 28.
 *      Author: noble
 */

#ifndef SRC_MAPPING_HOTMAP_HOTMAP_PGM_H_
#define SRC_MAPPING_HOTMAP_HOTMAP_PGM_H_

#include "../../pgm/pgm_index_dynamic.hpp"

#define DEBUG_HOTMAP 0

void hotmap_pgm_insert(pgm::DynamicPGMIndex<unsigned int, unsigned int> &hotmap,
                       unsigned int logicalSliceAddr,
                       unsigned int virtualSliceAddr);
unsigned int hotmap_pgm_find(
    pgm::DynamicPGMIndex<unsigned int, unsigned int> &hotmap,
    unsigned int logicalSliceAddr);
void hotmap_pgm_erase(pgm::DynamicPGMIndex<unsigned int, unsigned int> &hotmap,
                      unsigned int logicalSliceAddr);
void hotmap_pgm_update(pgm::DynamicPGMIndex<unsigned int, unsigned int> &hotmap,
                       unsigned int logicalSliceAddr,
                       unsigned int virtualSliceAddr);

#endif /* SRC_MAPPING_HOTMAP_HOTMAP_PGM_H_ */
