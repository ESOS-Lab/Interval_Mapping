/*
 * functional_mapping.h
 *
 *  Created on: 2021. 11. 04.
 *      Author: Minsu Jang (nobleminsu@gmail.com)
 */

#ifndef SRC_MAPPING_FUNCTIONAL_FUNCTIONAL_MAPPING_H_
#define SRC_MAPPING_FUNCTIONAL_FUNCTIONAL_MAPPING_H_

#include "../../alex/openssd_allocator.h"
#include "../mapseg/map_segment.h"

#define SLICES_ZONE (16 * (1 << 16))
#define SIZE_TEMP_NODE (1 << 4)
// remainder value to match zone size
#define SIZE_DATA_NODE (SLICES_ZONE / SIZE_TEMP_NODE / (MAPSEG_MAP_SEGMENT_SIZE))

#define FUNCTIONAL_MAPPING_TREE_COUNT 8
#define LSA_TO_TREE_NUM(lsa) ((lsa >> 27) & 7)
#define TREE_NUM_TO_FIRST_LSA(num) (num << 27)

#define POSITION_LESS (-1)
#define POSITION_LARGER

typedef struct node_model {
    unsigned int invSlope;
    unsigned int bias;
} NodeModel;

int calcPosition(NodeModel model, unsigned int logicalSliceAddr) {
    return (int)((logicalSliceAddr - model.bias) / model.invSlope);
}

typedef struct data_node {
    NodeModel model;
    int size;
    MapSegment *childMapSegments[SIZE_DATA_NODE];  // aka map segment
} DataNode;

typedef struct temp_node {
    NodeModel model;
    int size;
    DataNode childDataNodes[SIZE_TEMP_NODE];
} TempNode;

typedef struct root_node {
    NodeModel model;
    int size;                       // expandable
    TempNode *childTempNodes[128];  // temporary static allocation.
} RootNode;

typedef struct functional_mapping_tree {
    RootNode rootNode;
} FunctionalMappingTree;

extern FunctionalMappingTree fmTrees[FUNCTIONAL_MAPPING_TREE_COUNT];
extern OpenSSDAllocator<TempNode> tAllocator;
extern OpenSSDAllocator<MapSegment> mapSegmentAllocator;

inline void initDataNode(DataNode *node, unsigned int firstItemAddr) {
    node->size = SIZE_DATA_NODE;
    node->model.invSlope = MAPSEG_MAP_SEGMENT_SIZE;
    node->model.bias = firstItemAddr;
    memset(node->childMapSegments, 0, sizeof(MapSegment *) * SIZE_DATA_NODE);
}

inline void initRootNode(RootNode *node, unsigned int firstItemAddr) {
    node->size = 0;
    node->model.invSlope =
        SIZE_TEMP_NODE * SIZE_DATA_NODE * MAPSEG_MAP_SEGMENT_SIZE;
    node->model.bias = firstItemAddr;
}

inline void initTempNode(TempNode *node, unsigned int firstItemAddr) {
    node->size = SIZE_TEMP_NODE;
    node->model.invSlope = SIZE_DATA_NODE * MAPSEG_MAP_SEGMENT_SIZE;
    node->model.bias = firstItemAddr;

    unsigned int tempNodeAddr = node->model.bias;
    for (int i = 0; i < node->size; i++) {
        initDataNode(&node->childDataNodes[i], tempNodeAddr);
        tempNodeAddr += node->model.invSlope;
    }
}

inline int expandRootNode(RootNode *node, unsigned int targetAddr) {
    //	xil_printf("expantRoot %p\n", targetAddr);
    size_t targetItemIndex = calcPosition(node->model, targetAddr);

    // xil_printf("expantRoot %p, targetIdx %d, rootBias %p\n", targetAddr,
    //            targetItemIndex, node->model.bias);
    node->size = targetItemIndex + 1;
    // expand temp node
    // for now, use static

    // assume tempNode allocation only occurs on expansion
    // tempNode and underlying materials are initialized at once
    node->childTempNodes[targetItemIndex] = tAllocator.allocate(1);

    // init temp node
    unsigned int tempNodeAddr =
        node->model.invSlope * targetItemIndex + node->model.bias;
    initTempNode(node->childTempNodes[targetItemIndex], tempNodeAddr);

    return targetItemIndex;
}
inline void slideRootNode(RootNode *node) {}

inline TempNode *fetchTempNodeFromRootNode(RootNode *node,
                                           unsigned int logicalSliceAddr,
                                           int isAllocate) {
    int position = calcPosition(node->model, logicalSliceAddr);
    //    xil_printf("fetchRoot, addr=%p, position=%p, isAllocate=%d\n",
    //    logicalSliceAddr, position, isAllocate);
    if (position < 0) return NULL;
    if (position >= node->size) {
        if (!isAllocate) return NULL;
        position = expandRootNode(node, logicalSliceAddr);
    }

    //    xil_printf("fetchRoot yes, addr=%p, temp=%p\n", logicalSliceAddr,
    //    node->childTempNodes[position]);

    return node->childTempNodes[position];
}
inline DataNode *fetchDataNodeFromTempNode(TempNode *node,
                                           unsigned int logicalSliceAddr) {
    int position = calcPosition(node->model, logicalSliceAddr);
    //    xil_printf("fetchTemp, addr=%p, position=%p, nodesize=%d\n",
    //    logicalSliceAddr, position, node->size);
    if (position < 0 || position >= node->size) return NULL;
    //    xil_printf("fetchTemp yes, addr=%p, data=%p\n", logicalSliceAddr,
    //    &node->childDataNodes[position]);
    return &node->childDataNodes[position];
}

inline MapSegment *fetchMapSegmentFromDataNode(DataNode *node,
                                               unsigned int logicalSliceAddr,
                                               int isAllocate) {
    int position = calcPosition(node->model, logicalSliceAddr);
    //    xil_printf("fetchData, addr=%p, position=%p, nodesize=%d\n",
    //    logicalSliceAddr, position, node->size);
    if (position < 0 || position >= node->size) return NULL;
    MapSegment *pMapSegment = node->childMapSegments[position];
    if (pMapSegment == NULL && isAllocate) {
        pMapSegment = mapSegmentAllocator.allocate(1);
        node->childMapSegments[position] = pMapSegment;
        unsigned int mapSegmentStartAddr =
            position * node->model.invSlope + node->model.bias;
        mapseg_init_map_segment(pMapSegment, mapSegmentStartAddr);
        
    //    xil_printf("allocating, addr=%p, position=%p, addr=%p\n",
    //    logicalSliceAddr, position, mapSegmentStartAddr);
    }
    //    xil_printf("fetchData yes, addr=%p, chunk=%p\n", logicalSliceAddr, c);
    return pMapSegment;
}

inline MapSegment *fetchMapSegmentFromFmTree(FunctionalMappingTree *fmTree,
                                             unsigned int logicalSliceAddr,
                                             int isAllocateChunk) {
    TempNode *t = fetchTempNodeFromRootNode(&fmTree->rootNode, logicalSliceAddr,
                                            isAllocateChunk);
    if (t == NULL) return NULL;
    DataNode *d = fetchDataNodeFromTempNode(t, logicalSliceAddr);
    if (d == NULL) return NULL;
    MapSegment *c =
        fetchMapSegmentFromDataNode(d, logicalSliceAddr, isAllocateChunk);
    return c;
}
#endif /* SRC_MAPPING_FUNCTIONAL_FUNCTIONAL_MAPPING_H_ */
