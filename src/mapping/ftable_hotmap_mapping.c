/*
 * alex_ftable_mapping.c
 *
 *  Created on: 2021. 4. 28.
 *      Author: Minsu Jang (nobleminsu@gmail.com)
 */

#include "ftable_hotmap_mapping.h"

#include "../alex/alex.h"
#include "ftable/ftable.h"
#include "hotmap/hotmap.h"

alex::Alex<unsigned int, unsigned int> hotmap;
int curMaxFtableIdx = -1;
FTable hotFTables[FTABLE_DEFAULT_TABLE_NUM];

void fhm_migrate_ftable_entry_to_hotmap(unsigned int logicalSliceAddr,
                                        unsigned int virtualSliceAddr) {
    hotmap_insert(hotmap, logicalSliceAddr, virtualSliceAddr);
}

void fhm_insert(unsigned int logicalSliceAddr, unsigned int virtualSliceAddr) {
    FTable *ftable;

    int entryState = ftable_get_entry_state(logicalSliceAddr, hotFTables,
                                            FTABLE_DEFAULT_TABLE_NUM);
    if (entryState == FTABLE_ENTRY_ACTIVE) {
        ftable =
            ftable_select_table(logicalSliceAddr, hotFTables, curMaxFtableIdx);
        ftable_insert(ftable, logicalSliceAddr, virtualSliceAddr,
                      fhm_migrate_ftable_entry_to_hotmap);
    } else if (entryState == FTABLE_ENTRY_NOT_COVERED) {
        ftable =
            ftable_create_table(logicalSliceAddr, hotFTables, &curMaxFtableIdx,
                                FTABLE_DEFAULT_TABLE_NUM);
        ftable_insert(ftable, logicalSliceAddr, virtualSliceAddr,
                      fhm_migrate_ftable_entry_to_hotmap);
    } else if (entryState == FTABLE_ENTRY_SLIDED) {
        hotmap_insert(hotmap, logicalSliceAddr, virtualSliceAddr);
    }
}
unsigned int fhm_get(unsigned int sliceAddr) {
    int entryState =
        ftable_get_entry_state(sliceAddr, hotFTables, FTABLE_DEFAULT_TABLE_NUM);
    if (entryState == FTABLE_ENTRY_ACTIVE) {
        FTable *ftable =
            ftable_select_table(sliceAddr, hotFTables, curMaxFtableIdx);
        return ftable_get(ftable, sliceAddr);
    } else if (entryState == FTABLE_ENTRY_NOT_COVERED) {
        return VSA_NONE;
    } else if (entryState == FTABLE_ENTRY_SLIDED) {
        return hotmap_find(hotmap, sliceAddr);
    }
    return VSA_NONE;
}
void fhm_remove(unsigned int sliceAddr) {
    int entryState =
        ftable_get_entry_state(sliceAddr, hotFTables, FTABLE_DEFAULT_TABLE_NUM);
    if (entryState == FTABLE_ENTRY_ACTIVE) {
        FTable *ftable =
            ftable_select_table(sliceAddr, hotFTables, curMaxFtableIdx);
        ftable_invalidate(ftable, sliceAddr,
                          fhm_migrate_ftable_entry_to_hotmap);
    } else if (entryState == FTABLE_ENTRY_NOT_COVERED) {
    } else if (entryState == FTABLE_ENTRY_SLIDED) {
        hotmap_erase(hotmap, sliceAddr);
    }
}
void fhm_is_in_ftable(unsigned int sliceAddr) {}

void fhm_update(unsigned int logicalSliceAddr, unsigned int virtualSliceAddr) {
    int entryState =
        ftable_get_entry_state(logicalSliceAddr, hotFTables, FTABLE_DEFAULT_TABLE_NUM);
    if (entryState == FTABLE_ENTRY_ACTIVE) {
        FTable *ftable =
            ftable_select_table(logicalSliceAddr, hotFTables, curMaxFtableIdx);
        ftable_update(ftable, logicalSliceAddr, virtualSliceAddr);
    } else if (entryState == FTABLE_ENTRY_NOT_COVERED) {
    } else if (entryState == FTABLE_ENTRY_SLIDED) {
        hotmap_update(hotmap, logicalSliceAddr, virtualSliceAddr);
    }
}
