/*
 * ftable_hotmap_mapping.h
 *
 *  Created on: 2021. 4. 28.
 *      Author: Minsu Jang (nobleminsu@gmail.com)
 */

#ifndef SRC_FTABLE_HOTMAP_MAPPING_H_
#define SRC_FTABLE_HOTMAP_MAPPING_H_

void fhm_insert(unsigned int logicalSliceAddr, unsigned int virtualSliceAddr);
unsigned int fhm_get(unsigned int sliceAddr);
void fhm_remove(unsigned int sliceAddr);
void fhm_is_in_ftable(unsigned int sliceAddr);

#endif /* SRC_FTABLE_HOTMAP_MAPPING_H_ */
