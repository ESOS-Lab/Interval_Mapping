//////////////////////////////////////////////////////////////////////////////////
// nvme_io_cmd.c for Cosmos+ OpenSSD
// Copyright (c) 2016 Hanyang University ENC Lab.
// Contributed by Yong Ho Song <yhsong@enc.hanyang.ac.kr>
//				  Youngjin Jo <yjjo@enc.hanyang.ac.kr>
//				  Sangjin Lee <sjlee@enc.hanyang.ac.kr>
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
// Engineer: Sangjin Lee <sjlee@enc.hanyang.ac.kr>
//			 Jaewook Kwak <jwkwak@enc.hanyang.ac.kr>
//
// Project Name: Cosmos+ OpenSSD
// Design Name: Cosmos+ Firmware
// Module Name: NVMe IO Command Handler
// File Name: nvme_io_cmd.c
//
// Version: v1.0.1
//
// Description:
//   - handles NVMe IO command
//////////////////////////////////////////////////////////////////////////////////

//////////////////////////////////////////////////////////////////////////////////
// Revision History:
//
// * v1.0.1
//   - header file for buffer is changed from "ia_lru_buffer.h" to "lru_buffer.h"
//
// * v1.0.0
//   - First draft
//////////////////////////////////////////////////////////////////////////////////


#include "xil_printf.h"
#include "debug.h"
#include "io_access.h"

#include "nvme.h"
#include "host_lld.h"
#include "nvme_io_cmd.h"

#include "../ftl_config.h"
#include "../request_transform.h"

void handle_nvme_io_read(unsigned int cmdSlotTag, NVME_IO_COMMAND *nvmeIOCmd)
{
	IO_READ_COMMAND_DW12 readInfo12;
	//IO_READ_COMMAND_DW13 readInfo13;
	//IO_READ_COMMAND_DW15 readInfo15;
	unsigned int startLba[2];
	unsigned int nlb;

	readInfo12.dword = nvmeIOCmd->dword[12];
	//readInfo13.dword = nvmeIOCmd->dword[13];
	//readInfo15.dword = nvmeIOCmd->dword[15];

	/*
	 * struct nvme_rw_command {
		__u8			opcode;
		__u8			flags;
		__u16			command_id;
		__le32			nsid;
		__u64			rsvd2;
		__le64			metadata;
		union nvme_data_ptr	dptr;
		__le64			slba; // dword - 10, 11
		__le16			length; // dword - 12
		__le16			control; // dword - 12
		__le32			dsmgmt;
		__le32			reftag;
		__le16			apptag;
		__le16			appmask;
	};
	 */
	startLba[0] = nvmeIOCmd->dword[10]; // slba low bytes
	startLba[1] = nvmeIOCmd->dword[11]; // slba top bytes
	nlb = readInfo12.NLB;

	ASSERT(startLba[0] < storageCapacity_L && (startLba[1] < STORAGE_CAPACITY_H || startLba[1] == 0));
	//ASSERT(nlb < MAX_NUM_OF_NLB);
	ASSERT((nvmeIOCmd->PRP1[0] & 0xF) == 0 && (nvmeIOCmd->PRP2[0] & 0xF) == 0); //error
	ASSERT(nvmeIOCmd->PRP1[1] < 0x10 && nvmeIOCmd->PRP2[1] < 0x10);

	ReqTransNvmeToSlice(cmdSlotTag, startLba[0], nlb, IO_NVM_READ);
}


void handle_nvme_io_write(unsigned int cmdSlotTag, NVME_IO_COMMAND *nvmeIOCmd)
{
	IO_READ_COMMAND_DW12 writeInfo12;
	//IO_READ_COMMAND_DW13 writeInfo13;
	//IO_READ_COMMAND_DW15 writeInfo15;
	unsigned int startLba[2];
	unsigned int nlb;

	writeInfo12.dword = nvmeIOCmd->dword[12];
	//writeInfo13.dword = nvmeIOCmd->dword[13];
	//writeInfo15.dword = nvmeIOCmd->dword[15];

	//if(writeInfo12.FUA == 1)
	//	xil_printf("write FUA\r\n");

	startLba[0] = nvmeIOCmd->dword[10];
	startLba[1] = nvmeIOCmd->dword[11];
	nlb = writeInfo12.NLB;

	ASSERT(startLba[0] < storageCapacity_L && (startLba[1] < STORAGE_CAPACITY_H || startLba[1] == 0));
	//ASSERT(nlb < MAX_NUM_OF_NLB);
	ASSERT((nvmeIOCmd->PRP1[0] & 0xF) == 0 && (nvmeIOCmd->PRP2[0] & 0xF) == 0);
	ASSERT(nvmeIOCmd->PRP1[1] < 0x10 && nvmeIOCmd->PRP2[1] < 0x10);

	ReqTransNvmeToSlice(cmdSlotTag, startLba[0], nlb, IO_NVM_WRITE);
}

void handle_nvme_io_dataset_management(unsigned int cmdSlotTag, NVME_IO_COMMAND *nvmeIOCmd)
{
	IO_DATASET_MANAGEMENT_COMMAND_DW10 dsmInfo10;
	IO_DATASET_MANAGEMENT_COMMAND_DW11 dsmInfo11;

	dsmInfo10.dword = nvmeIOCmd->dword[10];
	dsmInfo11.dword = nvmeIOCmd->dword[11];

//	unsigned long long dsmAddr;
//	dsmAddr += ((unsigned long long)nvmeIOCmd->PRP1[1]) << 32;
//	dsmAddr += nvmeIOCmd->PRP1[0];
	xil_printf("dsm addr: %x:%x\n", nvmeIOCmd->PRP1[1], nvmeIOCmd->PRP1[0]);
	xil_printf("dsm data_len: %x\n", nvmeIOCmd->PRP2[1]);
	

	xil_printf("dsm 10: %x\n", dsmInfo10);
	xil_printf("dsm 11: %x\n", dsmInfo11);

	ReqHandleDatasetManagement(cmdSlotTag, dsmInfo10.NR, nvmeIOCmd->PRP1[1],nvmeIOCmd->PRP1[0],
								nvmeIOCmd->PRP2[1], dsmInfo11.AD);
}

void handle_nvme_io_cmd(NVME_COMMAND *nvmeCmd)
{
	NVME_IO_COMMAND *nvmeIOCmd;
	NVME_COMPLETION nvmeCPL;
	unsigned int opc;

	nvmeIOCmd = (NVME_IO_COMMAND*)nvmeCmd->cmdDword;
	opc = (unsigned int)nvmeIOCmd->OPC;

	switch(opc)
	{
		case IO_NVM_FLUSH:
		{
			xil_printf("IO Flush Command\r\n");
			nvmeCPL.dword[0] = 0;
			nvmeCPL.specific = 0x0;
			set_auto_nvme_cpl(nvmeCmd->cmdSlotTag, nvmeCPL.specific, nvmeCPL.statusFieldWord);
			break;
		}
		case IO_NVM_WRITE:
		{
			//xil_printf("IO Write Command\r\n");
			handle_nvme_io_write(nvmeCmd->cmdSlotTag, nvmeIOCmd);
			break;
		}
		case IO_NVM_READ:
		{
			//xil_printf("IO Read Command\r\n");
			handle_nvme_io_read(nvmeCmd->cmdSlotTag, nvmeIOCmd);
			break;
		}
		case IO_NVM_DATASET_MANAGEMENT:
		{
			xil_printf("IO Dsm Command\r\n");
			handle_nvme_io_dataset_management(nvmeCmd->cmdSlotTag, nvmeIOCmd);
			nvmeCPL.dword[0] = 0;
			nvmeCPL.specific = 0x0;
			set_auto_nvme_cpl(nvmeCmd->cmdSlotTag, nvmeCPL.specific, nvmeCPL.statusFieldWord);
			break;
		}
		default:
		{
			xil_printf("Not Support IO Command OPC: %X\r\n", opc);
			ASSERT(0);
			break;
		}
	}
}

