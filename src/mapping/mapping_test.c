/*
 * mapping_test.c
 *
 *  Created on: 2021. 5. 3.
 *      Author: Minsu Jang (nobleminsu@gmail.com)
 */

#include <assert.h>

#include "mapseg/map_segment.h"

// alex::Alex<unsigned int, unsigned int> testHotmap;
int testCurMaxFtableIdx = -1;
// FTable testHotFTables[FTABLE_DEFAULT_TABLE_NUM];

void test_fhm() {
    xil_printf("Starting fhm test...\n");
    xil_printf("fhm test ok.\n");
}
void test_hotmap() {}
void test_mapseg() {
    xil_printf("Starting ftable test...\n");

    mapseg_init();
    for (int i = 0; i < 30; i++) {
        xil_printf("setting %d\n", i);
        int isSetSuccess = mapseg_set_mapping(wchunkBucket, i, i + 1);
        xil_printf("getting %d\n", i);
        unsigned int out = mapseg_get_mapping(wchunkBucket, i);
        xil_printf("set success is %d, out is %d\n", isSetSuccess, out);
    }

    //    FTable* ftable = ftable_create_table(
    //        0, testHotFTables, &testCurMaxFtableIdx,
    //        FTABLE_DEFAULT_TABLE_NUM);
    //    ftable_insert(ftable, 0, 1, NULL);
    //    assert(ftable_get(ftable, 0) == 1);

    //    int i;
    //    for (i = 0; i < FTABLE_DEFAULT_CAPACITY * 10; i += 10000) {
    //        ftable_insert(ftable, i, i + 100, NULL);
    //        assert(ftable_get(ftable, i) == i + 100);
    //    }
    //    xil_printf("ftable test ok.\n");
}
