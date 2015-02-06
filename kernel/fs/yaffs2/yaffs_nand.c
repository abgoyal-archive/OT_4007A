/*
 * YAFFS: Yet Another Flash File System. A NAND-flash specific file system.
 *
 * Copyright (C) 2002-2010 Aleph One Ltd.
 *   for Toby Churchill Ltd and Brightstar Engineering
 *
 * Created by Charles Manning <charles@aleph1.co.uk>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 */
#include "yaffs_trace.h"
#include "yaffs_nand.h"
#include "yaffs_tagscompat.h"
#include "yaffs_tagsvalidity.h"

#include "yaffs_getblockinfo.h"

int yaffs_ReadChunkWithTagsFromNAND(yaffs_Device *dev, int chunkInNAND,
					   __u8 *buffer,
					   yaffs_ExtendedTags *tags)
{
	int result;
	yaffs_ExtendedTags localTags;

	int realignedChunkInNAND = chunkInNAND - dev->chunkOffset;

	dev->nPageReads++;

	/* If there are no tags provided, use local tags to get prioritised gc working */
	if (!tags)
		tags = &localTags;

	if (dev->param.readChunkWithTagsFromNAND)
		result = dev->param.readChunkWithTagsFromNAND(dev, realignedChunkInNAND, buffer,
						      tags);
	else
		result = yaffs_TagsCompatabilityReadChunkWithTagsFromNAND(dev,
									realignedChunkInNAND,
									buffer,
									tags);
	if (tags &&
	   tags->eccResult > YAFFS_ECC_RESULT_NO_ERROR) {

		yaffs_BlockInfo *bi;
		bi = yaffs_GetBlockInfo(dev, chunkInNAND/dev->param.nChunksPerBlock);
		yaffs_HandleChunkError(dev, bi);
	}

	return result;
}

int yaffs_WriteChunkWithTagsToNAND(yaffs_Device *dev,
						   int chunkInNAND,
						   const __u8 *buffer,
						   yaffs_ExtendedTags *tags)
{

	dev->nPageWrites++;

	chunkInNAND -= dev->chunkOffset;


	if (tags) {
		tags->sequenceNumber = dev->sequenceNumber;
		tags->chunkUsed = 1;
		if (!yaffs_ValidateTags(tags)) {
			T(YAFFS_TRACE_ERROR,
			  (TSTR("Writing uninitialised tags" TENDSTR)));
			YBUG();
		}
		T(YAFFS_TRACE_WRITE,
		  (TSTR("Writing chunk %d tags %d %d" TENDSTR), chunkInNAND,
		   tags->objectId, tags->chunkId));
	} else {
		T(YAFFS_TRACE_ERROR, (TSTR("Writing with no tags" TENDSTR)));
		YBUG();
	}

	if (dev->param.writeChunkWithTagsToNAND)
		return dev->param.writeChunkWithTagsToNAND(dev, chunkInNAND, buffer,
						     tags);
	else
		return yaffs_TagsCompatabilityWriteChunkWithTagsToNAND(dev,
								       chunkInNAND,
								       buffer,
								       tags);
}

int yaffs_MarkBlockBad(yaffs_Device *dev, int blockNo)
{
	blockNo -= dev->blockOffset;


	if (dev->param.markNANDBlockBad)
		return dev->param.markNANDBlockBad(dev, blockNo);
	else
		return yaffs_TagsCompatabilityMarkNANDBlockBad(dev, blockNo);
}

int yaffs_QueryInitialBlockState(yaffs_Device *dev,
						 int blockNo,
						 yaffs_BlockState *state,
						 __u32 *sequenceNumber)
{
	blockNo -= dev->blockOffset;

	if (dev->param.queryNANDBlock)
		return dev->param.queryNANDBlock(dev, blockNo, state, sequenceNumber);
	else
		return yaffs_TagsCompatabilityQueryNANDBlock(dev, blockNo,
							     state,
							     sequenceNumber);
}

void mtk_dump_byte(void *p, long count, unsigned long offset); 

static int yaffs_CheckEraseClean(struct yaffs_DeviceStruct *dev,
				int chunkInNAND,int page)
{
	int retval = YAFFS_OK;
	__u8 *data = yaffs_GetTempBuffer(dev, __LINE__);
	yaffs_ExtendedTags tags;
	int result;

	result = yaffs_ReadChunkWithTagsFromNAND(dev, chunkInNAND, data, &tags);

	if (tags.eccResult > YAFFS_ECC_RESULT_NO_ERROR)
		retval = YAFFS_FAIL;

	if (!yaffs_CheckFF(data, dev->nDataBytesPerChunk) || tags.chunkUsed) {
		T(YAFFS_TRACE_NANDACCESS,
		  (TSTR("Chunk %d not erased" TENDSTR), chunkInNAND));
		retval = YAFFS_FAIL;
//add debug by jinling.ke
		printk(KERN_ERR"yaffsdebug Erase CheckChunk chunk:%d addr:0x%x chunkUsed:%d page:%d\n",chunkInNAND,chunkInNAND*dev->nDataBytesPerChunk,tags.chunkUsed,page);

		//mtk_dump_byte(data,dev->nDataBytesPerChunk,0);		
		//printk(KERN_ERR"dump tag\n");
		mtk_dump_byte(&tags,sizeof(yaffs_ExtendedTags),0);
	}

	yaffs_ReleaseTempBuffer(dev, data, __LINE__);

	return retval;

}


int yaffs_EraseBlockInNAND(struct yaffs_DeviceStruct *dev,
				  int blockInNAND)
/*
{
	int result;

	blockInNAND -= dev->blockOffset;

	dev->nBlockErasures++;

	result = dev->param.eraseBlockInNAND(dev, blockInNAND);

	return result;
}
*/
{
	int result;
	int c;
	blockInNAND -= dev->blockOffset;

	dev->nBlockErasures++;

	result = dev->param.eraseBlockInNAND(dev, blockInNAND);
	
#ifdef YAFFS_MVG_TEST_ERASECHEKFF
	for(c=0;c<dev->param.nChunksPerBlock;c++)
	{
		yaffs_CheckEraseClean(dev,(blockInNAND+dev->blockOffset)*dev->param.nChunksPerBlock+c,c);
	}
#endif
	return result;
}

int yaffs_InitialiseNAND(struct yaffs_DeviceStruct *dev)
{
	if(dev->param.initialiseNAND)
		return dev->param.initialiseNAND(dev);
	return YAFFS_OK;
}



