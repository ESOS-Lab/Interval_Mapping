/*
 * alex_ftable_mapping.c
 *
 *  Created on: 2021. 4. 28.
 *      Author: Minsu Jang (nobleminsu@gmail.com)
 */

#include "ftable_hotmap_mapping.h"

#include "../alex/alex.h"
#include "../pgm/pgm_index_dynamic.hpp"
#include "ftable/ftable.h"
#include "hotmap/hotmap_alex.h"
#include "hotmap/hotmap_pgm.h"

#ifdef CUR_ALEX
alex::Alex<unsigned int, unsigned int> hotmap;
#endif
#ifdef CUR_PGM
pgm::DynamicPGMIndex<unsigned int, unsigned int> hotmap;
#endif
int curMaxFtableIdx = -1;
FTable hotFTables[FTABLE_DEFAULT_TABLE_NUM];

void fhm_migrate_ftable_entry_to_hotmap(unsigned int logicalSliceAddr,
                                        unsigned int virtualSliceAddr) {
#ifdef CUR_ALEX
    hotmap_insert(hotmap, logicalSliceAddr, virtualSliceAddr);
#endif
#ifdef CUR_PGM
    hotmap_pgm_insert(hotmap, logicalSliceAddr, virtualSliceAddr);
#endif
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
#ifdef CUR_ALEX
        hotmap_insert(hotmap, logicalSliceAddr, virtualSliceAddr);
#endif
#ifdef CUR_PGM
        hotmap_pgm_insert(hotmap, logicalSliceAddr, virtualSliceAddr);
#endif
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
#ifdef CUR_ALEX
        return hotmap_find(hotmap, sliceAddr);
#endif
#ifdef CUR_PGM
        return hotmap_pgm_find(hotmap, sliceAddr);
#endif
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
#ifdef CUR_ALEX
        hotmap_erase(hotmap, sliceAddr);
#endif
#ifdef CUR_PGM
        hotmap_pgm_erase(hotmap, sliceAddr);
#endif
    }
}
void fhm_is_in_ftable(unsigned int sliceAddr) {}

void fhm_update(unsigned int logicalSliceAddr, unsigned int virtualSliceAddr) {
    int entryState = ftable_get_entry_state(logicalSliceAddr, hotFTables,
                                            FTABLE_DEFAULT_TABLE_NUM);
    if (entryState == FTABLE_ENTRY_ACTIVE) {
        FTable *ftable =
            ftable_select_table(logicalSliceAddr, hotFTables, curMaxFtableIdx);
        ftable_update(ftable, logicalSliceAddr, virtualSliceAddr);
    } else if (entryState == FTABLE_ENTRY_NOT_COVERED) {
    } else if (entryState == FTABLE_ENTRY_SLIDED) {
#ifdef CUR_ALEX
        hotmap_update(hotmap, logicalSliceAddr, virtualSliceAddr);
#endif
#ifdef CUR_PGM
        hotmap_pgm_update(hotmap, logicalSliceAddr, virtualSliceAddr);
#endif
    }
}

void fhm_print_stats() {
#ifdef CUR_ALEX
    // struct Stats {
    // 	int num_keys = 0;
    // 	int num_model_nodes = 0;  // num model nodes
    // 	int num_data_nodes = 0;   // num data nodes
    // 	int num_expand_and_scales = 0;
    // 	int num_expand_and_retrains = 0;
    // 	int num_downward_splits = 0;
    // 	int num_sideways_splits = 0;
    // 	int num_model_node_expansions = 0;
    // 	int num_model_node_splits = 0;
    // 	long long num_downward_split_keys = 0;
    // 	long long num_sideways_split_keys = 0;
    // 	long long num_model_node_expansion_pointers = 0;
    // 	long long num_model_node_split_pointers = 0;
    // 	mutable long long num_node_lookups = 0;
    // 	mutable long long num_lookups = 0;
    // 	long long num_inserts = 0;
    // 	double splitting_time = 0;
    // 	double cost_computation_time = 0;
    // };
    alex::Alex<unsigned int, unsigned int>::Stats stats = hotmap.get_stats();
    xil_printf("num_keys=%d, num_model_nodes=%d, num_data_nodes=%d, num_splits=%d\n",
        stats.num_keys, stats.num_model_nodes, stats.num_data_nodes, stats.num_downward_splits + stats.num_sideways_splits);

#endif
}