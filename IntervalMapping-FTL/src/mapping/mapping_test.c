/*
 * mapping_test.c
 *
 *  Created on: 2021. 5. 3.
 *      Author: Minsu Jang (nobleminsu@gmail.com)
 */

#include <assert.h>

#include "mapseg/map_segment.h"
#include "xtime_l.h"

// alex::Alex<unsigned int, unsigned int> testHotmap;
int testCurMaxFtableIdx = -1;
// FTable testHotFTables[FTABLE_DEFAULT_TABLE_NUM];

void test_fhm() {
    xil_printf("Starting fhm test...\n");
    xil_printf("fhm test ok.\n");
}
void test_hotmap() {}
void test_mapseg() {
    xil_printf("Starting mapseg test...\n");

    XTime startTime;
    XTime mapTime;

    XTime_GetTime(&startTime);

    mapseg_init();
    for (unsigned int msb = 0; msb < 8; msb++)
        for (unsigned int i = 0; i < 10 * 16 * (1 << 16); i++) {
            //        xil_printf("setting %d\n", i);
            int isSetSuccess = mapseg_set_mapping((msb << 27) + i, i + 1, false);
            //        xil_printf("getting %d\n", i);
            unsigned int out = mapseg_get_mapping((msb << 27) + i);
            if (out != i + 1)
                xil_printf("set fail %d, out is %d\n", isSetSuccess, out);
        }

    XTime_GetTime(&mapTime);

    char outText[128];
    sprintf(outText, "mapseg test took %f sec\n",
            1.0 * (mapTime - startTime) / 500000000);
    xil_printf("%s", outText);

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
