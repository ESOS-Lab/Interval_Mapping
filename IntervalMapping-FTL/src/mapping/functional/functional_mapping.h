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
#define SIZE_ZONE_NODE (1 << 10)

#define FUNCTIONAL_MAPPING_TREE_COUNT 8
#define LSA_TO_TREE_NUM(lsa) ((lsa >> 29) & 7)
#define TREE_NUM_TO_FIRST_LSA(num) (num << 29)

#define POSITION_LESS (-1)
#define POSITION_LARGER

typedef struct node_model {
    unsigned int invSlope;
    unsigned int bias;
} NodeModel;

int calcPosition(NodeModel model, unsigned int logicalSliceAddr) {
    return (int)((logicalSliceAddr - model.bias) / model.invSlope);
}

struct root_node;

typedef struct zone_node {
    NodeModel model;
    int size;
    MapSegment *childMapSegments[SIZE_ZONE_NODE];  // aka map segment
    int usedCount;
    int invCount;  // if usedCount & invCount hits max, invalidate that
    // zone_node
    struct root_node *parent;
} ZoneNode;

typedef struct root_node {
    NodeModel model;
    int size;  // expandable
    ZoneNode **childZoneNodes;
    unsigned int mIntervalStartAddr;
    unsigned int mIntervalEndAddr;
    int aIntervalStartIndex;
    int aIntervalEndIndex;
} RootNode;

typedef struct functional_mapping_tree {
    RootNode *rootNode;
} FunctionalMappingTree;

extern FunctionalMappingTree fmTrees[FUNCTIONAL_MAPPING_TREE_COUNT];
extern OpenSSDAllocator<ZoneNode> tAllocator;
extern OpenSSDAllocator<MapSegment> mapSegmentAllocator;
extern OpenSSDAllocator<RootNode> rootNodeAllocator;
extern OpenSSDAllocator<ZoneNode *> rootNodeChildrenAllocator;

inline void initMappingTree(FunctionalMappingTree *fmTree) {
    fmTree->rootNode = rootNodeAllocator.allocate(1);
}

inline void initZoneNode(ZoneNode *node, unsigned int firstItemAddr) {
    node->size = SIZE_ZONE_NODE;
    node->model.invSlope = MAPSEG_MAP_SEGMENT_SIZE;
    node->model.bias = firstItemAddr;
    memset(node->childMapSegments, 0, sizeof(MapSegment *) * SIZE_ZONE_NODE);
    node->usedCount = 0;
    node->invCount = 0;
}

inline void initRootNode(RootNode *node, unsigned int firstItemAddr) {
    node->size = 0;
    node->model.invSlope = SIZE_ZONE_NODE * MAPSEG_MAP_SEGMENT_SIZE;
    node->model.bias = firstItemAddr;
    node->childZoneNodes = NULL;
    node->mIntervalStartAddr = firstItemAddr;
    node->mIntervalEndAddr = firstItemAddr + node->size * node->model.invSlope;
    node->aIntervalStartIndex = 0;
    node->aIntervalEndIndex = -1;
}

inline void tryInvalidateZoneNodes(RootNode *node) {
    // try to invalidate unnecessary zone nodes and update mapping interval
    xil_printf("tryInvalidate stIdx %d\n", node->aIntervalStartIndex);

    if (node->aIntervalStartIndex > 0) {
        node->size -= node->aIntervalStartIndex;
        node->model.bias += node->model.invSlope * node->aIntervalStartIndex;
        node->mIntervalStartAddr +=
            node->aIntervalStartIndex * node->model.invSlope;
        node->aIntervalEndIndex -= node->aIntervalStartIndex;
        node->aIntervalStartIndex = 0;
    }
}

inline void copyChildZoneNodes(ZoneNode **newZoneNodes,
                               int newActiveIntervalStartIndex,
                               int newRootNodeSize, ZoneNode **oldZoneNodes,
                               int oldActiveIntervalStartIndex,
                               int oldRootNodeSize) {
    // copy old root node's child pointers to new root nodes' child array.
    xil_printf("copy %d~%d to %d~%d\n", oldActiveIntervalStartIndex,
               oldRootNodeSize, newActiveIntervalStartIndex, newRootNodeSize);

    int oldI = oldActiveIntervalStartIndex;
    int newI = newActiveIntervalStartIndex;
    while (oldI < oldRootNodeSize && newI < newRootNodeSize) {
        newZoneNodes[newI] = oldZoneNodes[oldI];
        newI++;
        oldI++;
    }
}

inline void doExpandRootNode(RootNode *node, int count) {
    xil_printf("expandRoot from %d to %d\n", node->size, node->size + count);
    int oldAIntervalStartIndex = node->aIntervalStartIndex;
    int oldRootNodeSize = node->size;
    tryInvalidateZoneNodes(node);

    node->size += count;
    node->mIntervalEndAddr += count * node->model.invSlope;

    ZoneNode **oldZoneNodes = node->childZoneNodes;
    node->childZoneNodes = rootNodeChildrenAllocator.allocate(node->size);

    if (oldRootNodeSize > 0)
        copyChildZoneNodes(node->childZoneNodes, node->aIntervalStartIndex,
                           node->size, oldZoneNodes, oldAIntervalStartIndex,
                           oldRootNodeSize);
}

inline void expandRootNode(RootNode *node) {
    int expansionSize = 16;

    // twice if current active interval is smaller than 1024
    if (node->aIntervalEndIndex > 0 &&
        node->aIntervalEndIndex - node->aIntervalStartIndex + 1 < 1024)
        expansionSize =
            2 * (node->aIntervalEndIndex - node->aIntervalStartIndex + 1) -
            node->size;

    // expand until current active interval is included
    if ((node->size - 1) + expansionSize < node->aIntervalEndIndex) {
        expansionSize = node->aIntervalEndIndex - (node->size - 1) + 16;
    }

    doExpandRootNode(node, expansionSize);
}

#define THRESHOLD_ROOTNODE_EXPANSION 2

inline void extendRootNodeActiveInterval(RootNode *node, int position) {
    xil_printf("extendActiveInterval %d to %d\n", node->aIntervalEndIndex,
               position);
    int s = node->aIntervalEndIndex + 1;
    node->aIntervalEndIndex = position;
    // if size is not sufficient or threshold is reached, expand
    if (position > node->size - 1 ||
        (node->size - 1) - node->aIntervalEndIndex <
            THRESHOLD_ROOTNODE_EXPANSION)
        expandRootNode(node);

    // init new zone nodes until position
    // thus, we ensure that all zone nodes in the active interval is initialized
    for (; s <= position; s++) {
        xil_printf("initZoneNode %d\n", s);
        // allocate zone node
        node->childZoneNodes[s] = tAllocator.allocate(1);
        initZoneNode(node->childZoneNodes[s],
                     node->model.invSlope * s + node->model.bias);
    }
}

inline void slideRootNode(RootNode *node) {}

inline void incrementZoneNodeUsedCount(ZoneNode *node) { node->usedCount++; }
inline void incrementZoneNodeInvCount(ZoneNode *node) {
    // decrement zone node's children counter
    node->invCount++;
    // if usedCount == invCount == max, increment active interval's start index
    if (node->usedCount == SIZE_ZONE_NODE && node->invCount == SIZE_ZONE_NODE) {
        node->parent->aIntervalStartIndex++;
    }
}

inline ZoneNode *fetchZoneNodeFromRootNode(RootNode *node,
                                           unsigned int logicalSliceAddr,
                                           int isAllocate) {
	int position = calcPosition(node->model, logicalSliceAddr);
    //    xil_printf("fetchRoot, addr=%p, position=%p, isAllocate=%d\n",
    //               logicalSliceAddr, position, isAllocate);
    if (position < 0) return NULL;
    if (position > node->aIntervalEndIndex) {
        if (!isAllocate) return NULL;

        extendRootNodeActiveInterval(node, position);

    }

    // xil_printf("fetchRoot yes, addr=%p, temp=%p\n", logicalSliceAddr,
    //    node->childZoneNodes[position]);

    return node->childZoneNodes[position];
}

inline MapSegment *fetchMapSegmentFromDataNode(ZoneNode *node,
                                               unsigned int logicalSliceAddr,
                                               int isAllocate,
                                               uint32_t *parent_addr, bool* last_in_mapseg) {
    int position = calcPosition(node->model, logicalSliceAddr);
    *last_in_mapseg = (position == SIZE_ZONE_NODE - 1);
    //    xil_printf("fetchData, addr=%p, position=%p, nodesize=%d\n",
    //               logicalSliceAddr, position, node->size);
    if (position < 0 || position >= node->size) return NULL;
    MapSegment *pMapSegment = node->childMapSegments[position];
    *parent_addr = (uint32_t) ((MapSegment_p) (&(node->childMapSegments[position])));
    if (pMapSegment == NULL && isAllocate) {
        pMapSegment = mapSegmentAllocator.allocate(1);
        node->childMapSegments[position] = pMapSegment;
        unsigned int mapSegmentStartAddr =
            position * node->model.invSlope + node->model.bias;
        mapseg_init_map_segment(pMapSegment, mapSegmentStartAddr);
        pMapSegment->parent = node;
        incrementZoneNodeUsedCount(node);

//        xil_printf("allocating, addr=%p, position=%p, addr=%p, mapseg=%p size=%d\n",
//                   logicalSliceAddr, position, mapSegmentStartAddr,
//                   pMapSegment, sizeof(*pMapSegment));
    }
    //    xil_printf("fetchData yes, addr=%p, chunk=%p\n", logicalSliceAddr,
    //               pMapSegment);
    return pMapSegment;
}

inline MapSegment *fetchMapSegmentFromFmTree(FunctionalMappingTree *fmTree,
                                             unsigned int logicalSliceAddr,
                                             int isAllocateChunk,
                                             uint32_t *parent_addr_ret, bool* last_in_mapseg) {
	ZoneNode *d = fetchZoneNodeFromRootNode(fmTree->rootNode, logicalSliceAddr,
                                            isAllocateChunk);
    if (d == NULL) return NULL;
    uint32_t parent_addr;
    MapSegment *c = fetchMapSegmentFromDataNode(d, logicalSliceAddr,
                                                isAllocateChunk, &parent_addr, last_in_mapseg);
    if (parent_addr_ret != NULL) *parent_addr_ret = parent_addr;
    return c;
}
#endif /* SRC_MAPPING_FUNCTIONAL_FUNCTIONAL_MAPPING_H_ */
