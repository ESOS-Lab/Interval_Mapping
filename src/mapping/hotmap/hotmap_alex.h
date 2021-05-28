/*
 * hotmap.h
 *
 *  Created on: 2021. 4. 28.
 *      Author: noble
 */

#ifndef SRC_MAPPING_HOTMAP_HOTMAP_ALEX_H_
#define SRC_MAPPING_HOTMAP_HOTMAP_ALEX_H_

#include "../../alex/alex.h"

#define DEBUG_HOTMAP 0

void hotmap_insert(alex::Alex<unsigned int, unsigned int> &hotmap,
                   unsigned int logicalSliceAddr,
                   unsigned int virtualSliceAddr);
unsigned int hotmap_find(alex::Alex<unsigned int, unsigned int> &hotmap,
                         unsigned int logicalSliceAddr);
void hotmap_erase(alex::Alex<unsigned int, unsigned int> &hotmap,
                  unsigned int logicalSliceAddr);
void hotmap_update(alex::Alex<unsigned int, unsigned int> &hotmap,
                   unsigned int logicalSliceAddr,
                   unsigned int virtualSliceAddr);

#endif /* SRC_MAPPING_HOTMAP_HOTMAP_ALEX_H_ */
