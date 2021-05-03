/*
 * hotmap.h
 *
 *  Created on: 2021. 4. 28.
 *      Author: noble
 */

#ifndef SRC_MAPPING_HOTMAP_HOTMAP_H_
#define SRC_MAPPING_HOTMAP_HOTMAP_H_

#include "../../alex/alex.h"

extern alex::Alex<unsigned int, unsigned int> hotmap;

void hotmap_insert(alex::Alex<unsigned int, unsigned int> hotmap,
                   unsigned int logicalSliceAddr,
                   unsigned int virtualSliceAddr);
unsigned int hotmap_find(alex::Alex<unsigned int, unsigned int> hotmap,
                         unsigned int logicalSliceAddr);
unsigned int hotmap_erase(alex::Alex<unsigned int, unsigned int> hotmap,
                          unsigned int logicalSliceAddr);

#endif /* SRC_MAPPING_HOTMAP_HOTMAP_H_ */
