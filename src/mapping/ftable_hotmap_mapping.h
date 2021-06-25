/*
 * ftable_hotmap_mapping.h
 *
 *  Created on: 2021. 4. 28.
 *      Author: Minsu Jang (nobleminsu@gmail.com)
 */

#ifndef SRC_FTABLE_HOTMAP_MAPPING_H_
#define SRC_FTABLE_HOTMAP_MAPPING_H_

#define MODE_ALEX 0
#define MODE_PGM 1
#define CUR_MODE MODE_ALEX
#if CUR_MODE == MODE_PGM
#define CUR_PGM
#elif CUR_MODE == MODE_ALEX
#define CUR_ALEX
#endif

void fhm_insert(unsigned int logicalSliceAddr, unsigned int virtualSliceAddr);
unsigned int fhm_get(unsigned int sliceAddr);
void fhm_remove(unsigned int sliceAddr);
void fhm_is_in_ftable(unsigned int sliceAddr);
void fhm_update(unsigned int logicalSliceAddr, unsigned int virtualSliceAddr);
void fhm_print_stats();

#endif /* SRC_FTABLE_HOTMAP_MAPPING_H_ */
