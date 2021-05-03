/*
 * mapping_test.c
 *
 *  Created on: 2021. 5. 3.
 *      Author: Minsu Jang (nobleminsu@gmail.com)
 */

#include <assert.h>

#include "ftable/ftable.h"
#include "ftable_hotmap_mapping.h"
#include "hotmap/hotmap.h"

alex::Alex<unsigned int, unsigned int> testHotmap;
int testCurMaxFtableIdx = -1;
FTable testHotFTables[FTABLE_DEFAULT_TABLE_NUM];

void test_fhm() {}
void test_hotmap() {}
void test_ftable() {
    xil_printf("curcur=%d\n", testCurMaxFtableIdx);
    FTable* ftable = ftable_create_table(0, testHotFTables, &testCurMaxFtableIdx,
                                         FTABLE_DEFAULT_TABLE_NUM);
    ftable_insert(ftable, 0, 1);
    assert(ftable_get(ftable, 0) == 1);
}
