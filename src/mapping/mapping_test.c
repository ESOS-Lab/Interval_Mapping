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

void test_fhm() {
    xil_printf("Starting fhm test...\n");
    int i;
    for (i = 0; i < FTABLE_DEFAULT_CAPACITY * 2; i += 1) {
        fhm_insert(i, i + 100);
    }
    for (i = 0; i < FTABLE_DEFAULT_CAPACITY * 2; i += 1) {
        assert(fhm_get(i) == i + 100);
    }
    xil_printf("fhm test ok.\n");
}
void test_hotmap() {}
void test_ftable() {
    xil_printf("Starting ftable test...\n");
    FTable* ftable = ftable_create_table(
        0, testHotFTables, &testCurMaxFtableIdx, FTABLE_DEFAULT_TABLE_NUM);
    ftable_insert(ftable, 0, 1, NULL);
    assert(ftable_get(ftable, 0) == 1);

    int i;
    for (i = 0; i < FTABLE_DEFAULT_CAPACITY * 10; i += 10000) {
        ftable_insert(ftable, i, i + 100, NULL);
        assert(ftable_get(ftable, i) == i + 100);
    }
    xil_printf("ftable test ok.\n");
}
