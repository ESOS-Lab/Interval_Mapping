//////////////////////////////////////////////////////////////////////////////////
// garbage_collection.c for Cosmos+ OpenSSD
// Copyright (c) 2017 Hanyang University ENC Lab.
// Contributed by Yong Ho Song <yhsong@enc.hanyang.ac.kr>
//				  Jaewook Kwak <jwkwak@enc.hanyang.ac.kr>
//
// This file is part of Cosmos+ OpenSSD.
//
// Cosmos+ OpenSSD is free software; you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation; either version 3, or (at your option)
// any later version.
//
// Cosmos+ OpenSSD is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
// See the GNU General Public License for more details.
//
// You should have received a copy of the GNU General Public License
// along with Cosmos+ OpenSSD; see the file COPYING.
// If not, see <http://www.gnu.org/licenses/>.
//////////////////////////////////////////////////////////////////////////////////

//////////////////////////////////////////////////////////////////////////////////
// Company: ENC Lab. <http://enc.hanyang.ac.kr>
// Engineer: Jaewook Kwak <jwkwak@enc.hanyang.ac.kr>
//
// Project Name: Cosmos+ OpenSSD
// Design Name: Cosmos+ Firmware
// Module Name: Garbage Collector
// File Name: garbage_collection.c
//
// Version: v1.0.0
//
// Description:
//   - select a victim block
//   - collect valid pages to a free block
//   - erase a victim block to make a free block
//////////////////////////////////////////////////////////////////////////////////

//////////////////////////////////////////////////////////////////////////////////
// Revision History:
//
// * v1.0.0
//   - First draft
//////////////////////////////////////////////////////////////////////////////////

#include "xil_printf.h"
#include <assert.h>
#include "memory_map.h"
#include "mapping/mapseg/map_segment.h"

#include <string.h>
#include "xtime_l.h"
#include "xparameters.h"

P_GC_VICTIM_MAP gcVictimMapPtr;

void InitGcVictimMap()
{
	int dieNo, invalidSliceCnt;

	gcVictimMapPtr = (P_GC_VICTIM_MAP) GC_VICTIM_MAP_ADDR;

	for(dieNo=0 ; dieNo<USER_DIES; dieNo++)
	{
		for(invalidSliceCnt=0 ; invalidSliceCnt<SLICES_PER_BLOCK+1; invalidSliceCnt++)
		{
			gcVictimMapPtr->gcVictimList[dieNo][invalidSliceCnt].headBlock = BLOCK_NONE;
			gcVictimMapPtr->gcVictimList[dieNo][invalidSliceCnt].tailBlock = BLOCK_NONE;
		}
	}
}

int isGCRan = 0;
XTime lastReportedTime = 0;
int lastReportedGCBlocks = 0;
int lastReportedGCPages = 0; // 16K
int curGCBlocks = 0;
int curGCPages = 0;
XTime curTime;

void GarbageCollection(unsigned int dieNo)
{
	unsigned int victimBlockNo, pageNo, virtualSliceAddr, logicalSliceAddr, dieNoForGcCopy, reqSlotTag;
	XTime gcTime;

    if (!isGCRan) {
		XTime_GetTime(&gcTime);
        isGCRan = 1;
		char outText[32];
        sprintf(outText, "[%fs]GC has just started %d\n", 1.0 * gcTime / 500000000, dieNo);
		xil_printf("%s", outText);
    }

	victimBlockNo = GetFromGcVictimList(dieNo);
	dieNoForGcCopy = dieNo;

	if(virtualBlockMapPtr->block[dieNo][victimBlockNo].invalidSliceCnt != SLICES_PER_BLOCK)
	{
		for(pageNo=0 ; pageNo<USER_PAGES_PER_BLOCK ; pageNo++)
		{
			virtualSliceAddr = Vorg2VsaTranslation(dieNo, victimBlockNo, pageNo);
			logicalSliceAddr = virtualSliceMapPtr->virtualSlice[virtualSliceAddr].logicalSliceAddr;
//			logicalSliceAddr = virtualSlice.find(virtualSliceAddr).payload();

			if(logicalSliceAddr != LSA_NONE)
				if (mapseg_get_mapping(logicalSliceAddr) == virtualSliceAddr)
//				if (fhm_get(logicalSliceAddr) == virtualSliceAddr)
//				if (logicalSlice.find(logicalSliceAddr).payload() == virtualSliceAddr)
//				if(logicalSliceMapPtr->logicalSlice[logicalSliceAddr].virtualSliceAddr ==  virtualSliceAddr) //valid data
				{
					//read
					reqSlotTag = GetFromFreeReqQ();

					reqPoolPtr->reqPool[reqSlotTag].reqType = REQ_TYPE_NAND;
					reqPoolPtr->reqPool[reqSlotTag].reqCode = REQ_CODE_READ;
					reqPoolPtr->reqPool[reqSlotTag].logicalSliceAddr = logicalSliceAddr;
					reqPoolPtr->reqPool[reqSlotTag].reqOpt.dataBufFormat = REQ_OPT_DATA_BUF_TEMP_ENTRY;
					reqPoolPtr->reqPool[reqSlotTag].reqOpt.nandAddr = REQ_OPT_NAND_ADDR_VSA;
					reqPoolPtr->reqPool[reqSlotTag].reqOpt.nandEcc = REQ_OPT_NAND_ECC_ON;
					reqPoolPtr->reqPool[reqSlotTag].reqOpt.nandEccWarning = REQ_OPT_NAND_ECC_WARNING_OFF;
					reqPoolPtr->reqPool[reqSlotTag].reqOpt.rowAddrDependencyCheck = REQ_OPT_ROW_ADDR_DEPENDENCY_CHECK;
					reqPoolPtr->reqPool[reqSlotTag].reqOpt.blockSpace = REQ_OPT_BLOCK_SPACE_MAIN;
					reqPoolPtr->reqPool[reqSlotTag].dataBufInfo.entry = AllocateTempDataBuf(dieNo);
					UpdateTempDataBufEntryInfoBlockingReq(reqPoolPtr->reqPool[reqSlotTag].dataBufInfo.entry, reqSlotTag);
					reqPoolPtr->reqPool[reqSlotTag].nandInfo.virtualSliceAddr = virtualSliceAddr;

					SelectLowLevelReqQ(reqSlotTag);

					//write
					reqSlotTag = GetFromFreeReqQ();

					reqPoolPtr->reqPool[reqSlotTag].reqType = REQ_TYPE_NAND;
					reqPoolPtr->reqPool[reqSlotTag].reqCode = REQ_CODE_WRITE;
					reqPoolPtr->reqPool[reqSlotTag].logicalSliceAddr = logicalSliceAddr;
					reqPoolPtr->reqPool[reqSlotTag].reqOpt.dataBufFormat = REQ_OPT_DATA_BUF_TEMP_ENTRY;
					reqPoolPtr->reqPool[reqSlotTag].reqOpt.nandAddr = REQ_OPT_NAND_ADDR_VSA;
					reqPoolPtr->reqPool[reqSlotTag].reqOpt.nandEcc = REQ_OPT_NAND_ECC_ON;
					reqPoolPtr->reqPool[reqSlotTag].reqOpt.nandEccWarning = REQ_OPT_NAND_ECC_WARNING_OFF;
					reqPoolPtr->reqPool[reqSlotTag].reqOpt.rowAddrDependencyCheck = REQ_OPT_ROW_ADDR_DEPENDENCY_CHECK;
					reqPoolPtr->reqPool[reqSlotTag].reqOpt.blockSpace = REQ_OPT_BLOCK_SPACE_MAIN;
					reqPoolPtr->reqPool[reqSlotTag].dataBufInfo.entry = AllocateTempDataBuf(dieNo);
					UpdateTempDataBufEntryInfoBlockingReq(reqPoolPtr->reqPool[reqSlotTag].dataBufInfo.entry, reqSlotTag);
					reqPoolPtr->reqPool[reqSlotTag].nandInfo.virtualSliceAddr = FindFreeVirtualSliceForGc(dieNoForGcCopy, victimBlockNo);

					// TODO: update
					mapseg_set_mapping(logicalSliceAddr, reqPoolPtr->reqPool[reqSlotTag].nandInfo.virtualSliceAddr);
//					fhm_update(logicalSliceAddr, virtualSliceAddr);
//					*logicalSlice.get_payload(logicalSliceAddr) = reqPoolPtr->reqPool[reqSlotTag].nandInfo.virtualSliceAddr;
//					logicalSliceMapPtr->logicalSlice[logicalSliceAddr].virtualSliceAddr = reqPoolPtr->reqPool[reqSlotTag].nandInfo.virtualSliceAddr;
					virtualSliceMapPtr->virtualSlice[reqPoolPtr->reqPool[reqSlotTag].nandInfo.virtualSliceAddr].logicalSliceAddr = logicalSliceAddr;

					SelectLowLevelReqQ(reqSlotTag);

					curGCPages++;
				}
		}
	}

	EraseBlock(dieNo, victimBlockNo);


	curGCBlocks++;

	XTime_GetTime(&curTime);
	if (1.0 * (curTime - lastReportedTime) / (500000000) >= 10) {
		char reportString[1024];
		sprintf(reportString,
			"sec %f reporting curBlock %d curPage %d totalBlock %d totalPage %d\n",
			1.0 * curTime / (500000000),
			curGCBlocks, curGCPages, lastReportedGCBlocks + curGCBlocks, lastReportedGCPages + curGCPages);
		xil_printf("%s", reportString);

		lastReportedGCBlocks += curGCBlocks;
		lastReportedGCPages += curGCPages;
		lastReportedTime = curTime;
		curGCBlocks = 0;
		curGCPages = 0;
	}
}


void PutToGcVictimList(unsigned int dieNo, unsigned int blockNo, unsigned int invalidSliceCnt)
{
	if(gcVictimMapPtr->gcVictimList[dieNo][invalidSliceCnt].tailBlock != BLOCK_NONE)
	{
		virtualBlockMapPtr->block[dieNo][blockNo].prevBlock = gcVictimMapPtr->gcVictimList[dieNo][invalidSliceCnt].tailBlock;
		virtualBlockMapPtr->block[dieNo][blockNo].nextBlock = BLOCK_NONE;
		virtualBlockMapPtr->block[dieNo][gcVictimMapPtr->gcVictimList[dieNo][invalidSliceCnt].tailBlock].nextBlock = blockNo;
		gcVictimMapPtr->gcVictimList[dieNo][invalidSliceCnt].tailBlock = blockNo;
	}
	else
	{
		virtualBlockMapPtr->block[dieNo][blockNo].prevBlock = BLOCK_NONE;
		virtualBlockMapPtr->block[dieNo][blockNo].nextBlock = BLOCK_NONE;
		gcVictimMapPtr->gcVictimList[dieNo][invalidSliceCnt].headBlock = blockNo;
		gcVictimMapPtr->gcVictimList[dieNo][invalidSliceCnt].tailBlock = blockNo;
	}
}

unsigned int GetFromGcVictimList(unsigned int dieNo)
{
	unsigned int evictedBlockNo;
	int invalidSliceCnt;

	for(invalidSliceCnt = SLICES_PER_BLOCK; invalidSliceCnt > 0 ; invalidSliceCnt--)
	{
		if(gcVictimMapPtr->gcVictimList[dieNo][invalidSliceCnt].headBlock != BLOCK_NONE)
		{
			evictedBlockNo = gcVictimMapPtr->gcVictimList[dieNo][invalidSliceCnt].headBlock;

			if(virtualBlockMapPtr->block[dieNo][evictedBlockNo].nextBlock != BLOCK_NONE)
			{
				virtualBlockMapPtr->block[dieNo][virtualBlockMapPtr->block[dieNo][evictedBlockNo].nextBlock].prevBlock = BLOCK_NONE;
				gcVictimMapPtr->gcVictimList[dieNo][invalidSliceCnt].headBlock = virtualBlockMapPtr->block[dieNo][evictedBlockNo].nextBlock;

			}
			else
			{
				gcVictimMapPtr->gcVictimList[dieNo][invalidSliceCnt].headBlock = BLOCK_NONE;
				gcVictimMapPtr->gcVictimList[dieNo][invalidSliceCnt].tailBlock = BLOCK_NONE;
			}
			return evictedBlockNo;

		}
	}

	assert(!"[WARNING] There are no free blocks. Abort terminate this ssd. [WARNING]");
	return BLOCK_FAIL;
}


void SelectiveGetFromGcVictimList(unsigned int dieNo, unsigned int blockNo)
{
	unsigned int nextBlock, prevBlock, invalidSliceCnt;

	nextBlock = virtualBlockMapPtr->block[dieNo][blockNo].nextBlock;
	prevBlock = virtualBlockMapPtr->block[dieNo][blockNo].prevBlock;
	invalidSliceCnt = virtualBlockMapPtr->block[dieNo][blockNo].invalidSliceCnt;

	if((nextBlock != BLOCK_NONE) && (prevBlock != BLOCK_NONE))
	{
		virtualBlockMapPtr->block[dieNo][prevBlock].nextBlock = nextBlock;
		virtualBlockMapPtr->block[dieNo][nextBlock].prevBlock = prevBlock;
	}
	else if((nextBlock == BLOCK_NONE) && (prevBlock != BLOCK_NONE))
	{
		virtualBlockMapPtr->block[dieNo][prevBlock].nextBlock = BLOCK_NONE;
		gcVictimMapPtr->gcVictimList[dieNo][invalidSliceCnt].tailBlock = prevBlock;
	}
	else if((nextBlock != BLOCK_NONE) && (prevBlock == BLOCK_NONE))
	{
		virtualBlockMapPtr->block[dieNo][nextBlock].prevBlock = BLOCK_NONE;
		gcVictimMapPtr->gcVictimList[dieNo][invalidSliceCnt].headBlock = nextBlock;
	}
	else
	{
		gcVictimMapPtr->gcVictimList[dieNo][invalidSliceCnt].headBlock = BLOCK_NONE;
		gcVictimMapPtr->gcVictimList[dieNo][invalidSliceCnt].tailBlock = BLOCK_NONE;
	}
}

