/**
Copyright 2013 3DSGuy

This file is part of extdata_tool.

extdata_tool is free software: you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation, either version 3 of the License, or
(at your option) any later version.

extdata_tool is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with extdata_tool. If not, see <http://www.gnu.org/licenses/>.
**/
#include "lib.h"
#include "extdata.h"
#include "ctr_crypto.h"

static ExtdataHashTable hashtable;
static ExtdataWorkingCTX ext_ctx;

// Private Prototypes
int VerifyIVFCLevels(void);

int GenerateIVFCHashTree(CreateExtdataCTX *ctx, u8 *outbuff);
int GenerateDIFIPartitions(CreateExtdataCTX *ctx, u8 *outbuff);
int GenerateDIFIPartition(u32 partition, DIFI_PARTITION *part, CreateExtdataCTX *ctx);
int GenerateDIFFHeader(CreateExtdataCTX *ctx, u8 *outbuff);

// Code
void InitaliseExtdataContext(ExtdataContext *ctx)
{
	memset(ctx,0,sizeof(ExtdataContext));
}

int GetExtdataContext(ExtdataContext *ctx)
{
	memset(&ext_ctx,0,sizeof(ExtdataWorkingCTX));
	ext_ctx.extdata = ctx->extdata.buffer;
	ext_ctx.Size = ctx->extdata.size;
	ext_ctx.Verbose = ctx->Verbose;	

	//Getting Data from DIFF
	DIFF_STRUCT *diff = (DIFF_STRUCT*)(ext_ctx.extdata+0x100);
	if(u8_to_u32(diff->magic_0,BE) != DIFF_MAGIC_0 || u8_to_u32(diff->magic_1,BE) != DIFF_MAGIC_1){
		return NOT_EXTDATA;
	}
	ext_ctx.ActivePartition = u8_to_u32(diff->active_partition,LE);
	if(ctx->OverrideActiveDIFI) ext_ctx.ActivePartition = ctx->DIFIPartition;
	ext_ctx.DIFIPartitions[Primary] = u8_to_u64(diff->primary_partition_offset,LE);
	ext_ctx.DIFIPartitions[Secondary] = u8_to_u64(diff->secondary_partition_offset,LE);
	ext_ctx.DIFIPartitionSize = u8_to_u64(diff->partition_table_length,LE);
	ext_ctx.FileBaseOffset = u8_to_u64(diff->file_base_offset,LE);
	ext_ctx.FileBaseSize = u8_to_u64(diff->file_base_size,LE);
	
	if(memcmp(ctx->VSXE_Extdata_ID,diff->extdata_unique_id,0x8) == Good) ctx->VSXE_Extdata_ID_Match = True;
	if(ext_ctx.Verbose) {
		memdump(stdout,"[+] Actual Extdata Unique ID:   ",diff->extdata_unique_id,8);
		memdump(stdout,"[+] Expected Extdata Unique ID: ",ctx->VSXE_Extdata_ID,8);
	}
	
	// Getting Data from Active DIFI
	u8 ActualActivePartitionHash[0x20];
	ctr_sha(ext_ctx.extdata+ext_ctx.DIFIPartitions[ext_ctx.ActivePartition],ext_ctx.DIFIPartitionSize,ActualActivePartitionHash,CTR_SHA_256);
	if(memcmp(ActualActivePartitionHash,diff->active_partition_hash,0x20) != 0){
		hashtable.ActivePartitionValid++;
		hashtable.TrustChain++;
	}
	DIFI_PARTITION *partition = (DIFI_PARTITION*)(ext_ctx.extdata+ext_ctx.DIFIPartitions[ext_ctx.ActivePartition]);
	if(u8_to_u32(partition->DIFI.magic_0,BE) != DIFI_MAGIC_0 || u8_to_u32(partition->DIFI.magic_1,BE) != DIFI_MAGIC_1)
		return NOT_EXTDATA;
	if(u8_to_u32(partition->IVFC.magic_0,BE) != IVFC_MAGIC_0 || u8_to_u32(partition->IVFC.magic_1,BE) != IVFC_MAGIC_1 )
		return IVFC_CORRUPT;
	if(u8_to_u32(partition->DPFS.magic_0,BE) != DPFS_MAGIC_0 || u8_to_u32(partition->DPFS.magic_1,BE) != DPFS_MAGIC_1 )
		return DPFS_CORRUPT;
		
	ext_ctx.DataOffset = 0;
	if(partition->DIFI.flags[0] == 1){
		ext_ctx.DataOffset = u8_to_u64(partition->DIFI.data_partition_offset,LE) + ext_ctx.FileBaseOffset;
		ext_ctx.IsDATA = True;
		ctx->ExtdataType = DataPartition;
	}
	
	ext_ctx.IVFC_Master_Hash_Blob_Offset = u8_to_u64(partition->DIFI.hash_blob_offset,LE) + ext_ctx.DIFIPartitions[ext_ctx.ActivePartition];
	ext_ctx.IVFC_Master_Hash_Blob_Size = u8_to_u64(partition->DIFI.hash_blob_size,LE);
	
	// Processing DPFS Blob
	for(int i = 0; i < 3; i++){
		ext_ctx.DPFS_Table_Offset[i] = u8_to_u64(partition->DPFS.table[i].offset,LE);
		ext_ctx.DPFS_Table_Size[i] = u8_to_u64(partition->DPFS.table[i].size,LE);
		ext_ctx.DPFS_Table_BLK_Size[i] = 1 << u8_to_u64(partition->DPFS.table[i].block_size,LE);
	}
	
	ext_ctx.IVFC_Offset = ext_ctx.DPFS_Table_Offset[tableivfc] + ext_ctx.FileBaseOffset;
	if(partition->DIFI.flags[1] == 1) ext_ctx.IVFC_Offset += ext_ctx.DPFS_Table_Size[tableivfc];
	
	// Processing IVFC Blob
	for(int i = 0; i < 4; i++){
		ext_ctx.IVFC_Lvl_Offset[i] = u8_to_u64(partition->IVFC.level[i].relative_offset,LE) + ext_ctx.IVFC_Offset;
		ext_ctx.IVFC_Lvl_Size[i] = u8_to_u64(partition->IVFC.level[i].size,LE);
		ext_ctx.IVFC_Lvl_BLK_Size[i] = 1 << u8_to_u64(partition->IVFC.level[i].block_size,LE);
	}

	// Verifying IVFC Levels
	VerifyIVFCLevels();
	
	// Getting Offsets Embedded Data
	ctx->Files.Count = 1;
	ctx->Files.Data = malloc(sizeof(ExtdataFileSectionData)*ctx->Files.Count);
	if(ctx->Files.Data == NULL)
		return MEM_ERROR;
	ctx->Files.Data[0].offset = ext_ctx.IVFC_Lvl_Offset[level4];
	if(ext_ctx.IsDATA) ctx->Files.Data[0].offset = ext_ctx.DataOffset;
	ctx->Files.Data[0].size = ext_ctx.IVFC_Lvl_Size[level4];	
	
	// Setting Trust Chain in input context
	ctx->TrustChain = hashtable.TrustChain;
	
	if(ctx->PrintInfo){
		memdump(stdout,"AES MAC:                  ",ext_ctx.extdata,0x10);
		printf("DIFF Header\n");
		printf(" > Active DIFI Data:\n");   
		switch(ext_ctx.ActivePartition){
			case Primary: printf("    Partition:            Primary\n"); break;
			case Secondary: printf("    Partition:            Secondary\n"); break;
		}
		switch(hashtable.ActivePartitionValid){
			case Good: memdump(stdout,"    Hash (GOOD):          ",diff->active_partition_hash,0x20); break;
			case Fail: memdump(stdout,"    Hash (FAIL):          ",diff->active_partition_hash,0x20); break;
		}
		printf(" > File Base Offset:      0x%llx\n",ext_ctx.FileBaseOffset);
		printf(" > File Base Size:        0x%llx\n",ext_ctx.FileBaseSize);
		memdump(stdout," > VSXE Extdata ID:       ",diff->extdata_unique_id,0x8);
		printf("DIFI Partition Table\n");
		if(ext_ctx.IsDATA) printf(" > DATA Offset:    0x%llx\n",ext_ctx.DataOffset);
		
		printf(" > IVFC\n");
		for(int i = 0; i < 4; i++){
			printf(" Level %d",i+1);
			if(i==level4) printf(" (FileSystem)");
			if(hashtable.IVFC_Level_Validity[i] == 0) printf(" (GOOD)");
			else printf(" (FAIL)");
			printf(":\n");
			printf("    Offset:        0x%llx\n",ext_ctx.IVFC_Lvl_Offset[i]);
			printf("    Size:          0x%llx\n",ext_ctx.IVFC_Lvl_Size[i]);
			printf("    BLKSize:       0x%llx\n",ext_ctx.IVFC_Lvl_BLK_Size[i]);
		}
		
		printf(" > DPFS\n");
		for(int i = 0; i < 3; i++){
			printf(" Table %d",i+1);
			if(i==tableivfc) printf(" (IVFC)");
			printf(":\n");
			printf("    Offset:        0x%llx\n",ext_ctx.DPFS_Table_Offset[i]);
			printf("    Size:          0x%llx\n",ext_ctx.DPFS_Table_Size[i]);
			printf("    BLKSize:       0x%llx\n",ext_ctx.DPFS_Table_BLK_Size[i]);
		}
	}
	return 0;
}

int VerifyIVFCLevels(void)
{
	for(int i = 0; i < 4; i++){
		if(ext_ctx.Verbose)printf("Checking Level %d\n",i+1);
		// Getting Previous IVFC level data		
		u64 prev_offset = ext_ctx.IVFC_Master_Hash_Blob_Offset;
		u64 prev_size = ext_ctx.IVFC_Master_Hash_Blob_Size;
		if(i > level1){
			prev_offset = ext_ctx.IVFC_Lvl_Offset[i-1];
			prev_size = ext_ctx.IVFC_Lvl_Size[i-1];
		}
		u32 hashcount = (prev_size/0x20);
		
		// Getting IVFC level data
		u64 offset = ext_ctx.IVFC_Lvl_Offset[i];
		u64 size = ext_ctx.IVFC_Lvl_Size[i];
		u64 blocksize = ext_ctx.IVFC_Lvl_BLK_Size[i];;
		u32 blocknum = (align_value(size,blocksize) / blocksize);
		
		if(i == level4 && ext_ctx.IsDATA) offset = ext_ctx.DataOffset;
		
		// The number of blocks in the current level must match the number of hashes in the previous level
		if(hashcount != blocknum){ 
			printf("[!] Unexpected difference in hashcount (%d) and block num (%d) for level (%d)\n",hashcount,blocknum,i+1);
			return Fail;
		}
		
		// Allocating IVFC level block
		u8 *IVFC_Block = malloc(blocksize);
		if(IVFC_Block == NULL){
			printf("[!] Failed to allocate memory for checking IVFC Level %d\n",i+1);
			return Fail;
		}
				
		u8 EmptyHash[0x20];
		memset(EmptyHash,0,0x20);
				
		// Checking each block in current IVFC level
		for(int j = 0; j < blocknum; j++){
			if(ext_ctx.Verbose) printf("  Checking Block %d\n",j);
						
			u8 *ExpectedHash = ext_ctx.extdata + prev_offset + (j*0x20);
			u8 *DataToHash = ext_ctx.extdata + offset + (j*blocksize);
			
			// Creating Block if remaining data is less than the block size
			if(j == blocknum - 1){
				u64 BlockReadSize = size - (blocksize*(blocknum-1));
				memset(IVFC_Block,0,blocksize);
				memcpy(IVFC_Block,DataToHash,BlockReadSize);
				DataToHash = IVFC_Block;
			}
			
			u8 ActualHash[0x20];
			ctr_sha(DataToHash,blocksize,ActualHash,CTR_SHA_256);			
						
			// Storing result
			if(memcmp(EmptyHash,ExpectedHash,0x20) != 0){
				hashtable.IVFC_Level_Validity[i] += memcmp(ActualHash,ExpectedHash,0x20);
				if(ext_ctx.Verbose){
					memdump(stdout,"  Expected Hash:   ",ExpectedHash,0x20);
					memdump(stdout,"  Actual Hash:     ",ActualHash,0x20);
				
				}
			}
			//else{
			//	printf("  Good\n");
			//}
		}
		
		// Freeing block
		free(IVFC_Block);
		
		// Updating Overall Chain of Trust Counter
		hashtable.TrustChain += hashtable.IVFC_Level_Validity[i];
	}
	return 0;
}

int CreateExtdata(COMPONENT_STRUCT *out, COMPONENT_STRUCT *in, int type, u32 active_difi, u8 unique_extdata_id[8])
{
	if(!in->size){
		printf("[!] In data has no size");
		return Fail;
	}
	
	CreateExtdataCTX ctx;
	memset(&ctx,0,sizeof(CreateExtdataCTX));
	// Get total size, and predict other values
	ctx.ExtdataType = type;
	ctx.ActiveDIFI = active_difi;
	memcpy(ctx.UniqueExtdataID,unique_extdata_id,8);
	
	
	// Getting IVFC Level Block Sizes
	ctx.IVFC_BLK_SIZE[level1] = 0x200;
	ctx.IVFC_BLK_SIZE[level2] = 0x200;
	ctx.IVFC_BLK_SIZE[level3] = 0x1000;
	ctx.IVFC_BLK_SIZE[level4] = 0x1000;
	if(type == BUILD_DB){
		ctx.IVFC_BLK_SIZE[level3] = 0x200;
		ctx.IVFC_BLK_SIZE[level4] = 0x200;
	}
	
	// Getting IVFC Level Sizes
	ctx.IVFC_SIZE[level4] = in->size;
	for(int i = level3; i >= level1; i--)
		ctx.IVFC_SIZE[i] = (align_value(ctx.IVFC_SIZE[i+1],ctx.IVFC_BLK_SIZE[i+1])/ctx.IVFC_BLK_SIZE[i+1])*0x20;
	
	// Getting Master Hash Blob size;
	ctx.IVFC_MASTER_HASH.size = ((align_value(ctx.IVFC_SIZE[level1],ctx.IVFC_BLK_SIZE[level1]) / ctx.IVFC_BLK_SIZE[level1]))*0x20;
	ctx.IVFC_MASTER_HASH.buffer = malloc(ctx.IVFC_MASTER_HASH.size);
	
	// Getting DIFI Partition Size/Offsets
	ctx.PARTITION_SIZE = sizeof(DIFI_PARTITION) + ctx.IVFC_MASTER_HASH.size;
	ctx.PARTITION_OFFSET[Secondary] = 0x100 + sizeof(DIFF_STRUCT);
	ctx.PARTITION_OFFSET[Primary] = ctx.PARTITION_OFFSET[Secondary] + align_value(ctx.PARTITION_SIZE,0x10);
	
	// Getting IVFC Relative Offsets
	ctx.IVFC_OFFSET[level1] = 0;
	for(int i = level2; i <= level4; i++){
		ctx.IVFC_OFFSET[i] = ctx.IVFC_SIZE[i-1] + ctx.IVFC_OFFSET[i-1];
		if(ctx.IVFC_SIZE[i] > ctx.IVFC_BLK_SIZE[i] && ctx.IVFC_SIZE[i] >= 0x4000 /*Seems to be the case*/)
			ctx.IVFC_OFFSET[i] = align_value(ctx.IVFC_OFFSET[i],ctx.IVFC_BLK_SIZE[i]);
	}

	// Getting File Base Offset
	ctx.FB_OFFSET = align_value(ctx.PARTITION_OFFSET[Primary] + align_value(ctx.PARTITION_SIZE,0x10),0x1000);
	if(type == BUILD_DB) ctx.FB_OFFSET = 0x600;
	
	// Getting DPFS Blob
	ctx.TableOffset[table1] = 0x0;
	ctx.TableSize[table1] = 0x4;
	ctx.TableBLKSize[table1] = 0x1;	
	ctx.TableOffset[table2] = 0x8;
	ctx.TableSize[table2] = 0x80;
	ctx.TableBLKSize[table2] = 0x80;
	ctx.TableOffset[tableivfc] = 0x1000;
	ctx.TableBLKSize[tableivfc] = 0x1000;
	if(type == BUILD_DB){
		ctx.TableOffset[tableivfc] = 0x200;
		ctx.TableBLKSize[tableivfc] = 0x200;
	}
	if(type == BUILD_DATA){
		ctx.TableSize[tableivfc] = align_value(ctx.IVFC_OFFSET[level3]+ctx.IVFC_SIZE[level3],ctx.IVFC_BLK_SIZE[level3]);
		ctx.DATA_OFFSET = ctx.TableOffset[tableivfc] + ctx.TableSize[tableivfc]*2;
		ctx.FB_SIZE = ctx.DATA_OFFSET + ctx.IVFC_SIZE[level4];
		ctx.TotalSize = ctx.FB_OFFSET + ctx.FB_SIZE;
		ctx.AbsoluteFileOffset = ctx.FB_OFFSET + ctx.DATA_OFFSET;
	}
	else{
		ctx.TableSize[tableivfc] = align_value(ctx.IVFC_OFFSET[level4]+ctx.IVFC_SIZE[level4],ctx.TableBLKSize[tableivfc]);
		ctx.FB_SIZE = ctx.FB_OFFSET + ctx.TableSize[tableivfc]*2;
		ctx.TotalSize = ctx.FB_OFFSET + ctx.FB_SIZE;
		ctx.AbsoluteFileOffset = ctx.FB_OFFSET + ctx.TableOffset[tableivfc] + ctx.IVFC_OFFSET[level4];
	}
	
	/**
	for(int i = 0; i < 4; i++){
		printf("IVFC Level %d\n",i+1);
		printf(" > Offset:     0x%llx\n",ctx.IVFC_OFFSET[i]);
		printf(" > Size:       0x%llx\n",ctx.IVFC_SIZE[i]);
		printf(" > BLKSize:    0x%llx\n",ctx.IVFC_BLK_SIZE[i]);
	}
	
	printf("DPFS IVFC Size: 0x%llx\n",ctx.TableSize[tableivfc]);
	
	printf("DATA Offset:    0x%llx\n",ctx.DATA_OFFSET);
	
	printf("FB SIZE:        0x%llx\n",ctx.FB_SIZE);
	printf("Abs Offset:     0x%llx\n",ctx.AbsoluteFileOffset);
	**/
	
	// Allocate
	out->size = ctx.TotalSize;
	out->buffer = malloc(out->size);
	if(out->buffer == NULL){
		printf("[!] Failed to allocate memory for creating extdata\n");
		return 1;
	}
	memset(out->buffer,0,out->size);
	
	// Get IVFC Hash Tree
	memcpy(out->buffer+ctx.AbsoluteFileOffset,in->buffer,ctx.IVFC_SIZE[level4]);
	GenerateIVFCHashTree(&ctx,out->buffer);
	
	// Generate DIFI Partitions
	GenerateDIFIPartitions(&ctx,out->buffer);
	
	// Generate DIFF Header
	GenerateDIFFHeader(&ctx,out->buffer);
	
	// Set MAC to 0xFF
	memset(out->buffer,0xff,0x10);
	
	return 0;
}

int GenerateIVFCHashTree(CreateExtdataCTX *ctx, u8 *outbuff)
{
	u64 IVFC_FILE_OFFSET = ctx->FB_OFFSET + ctx->TableOffset[tableivfc];
	for(int i = level4; i >= level1; i--){
		u64 prev_offset = 0;
		u64 prev_size = 0;
		u32 hashcount = 1;
		// Getting Previous Level details
		if(i > level1){
			prev_offset = IVFC_FILE_OFFSET + ctx->IVFC_OFFSET[i-1];
			prev_size = ctx->IVFC_SIZE[i-1];
			hashcount = (prev_size/0x20);
		}
		// Getting Current IVFC level data
		u64 offset = IVFC_FILE_OFFSET + ctx->IVFC_OFFSET[i];
		if(i==level4) offset = ctx->AbsoluteFileOffset;
		u64 size = ctx->IVFC_SIZE[i];
		u64 blocksize = ctx->IVFC_BLK_SIZE[i];
		u32 blocknum = (align_value(size,blocksize) / blocksize);
		
		
		
		// The number of blocks in the current level must match the number of hashes in the previous level
		if(hashcount != blocknum && i > level1){ 
			printf("[!] Unexpected difference in hashcount (%d) and block num (%d) for level (%d)\n",hashcount,blocknum,i+1);
			//return Fail;
		}
		
		// Allocating IVFC level block
		u8 *IVFC_Block = malloc(blocksize);
		if(IVFC_Block == NULL){
			printf("[!] Failed to allocate memory for checking IVFC Level %d\n",i+1);
			return Fail;
		}
		memset(IVFC_Block,0,blocksize);
		u8 EmptyBlockHash[0x20];
		ctr_sha(IVFC_Block,blocksize,EmptyBlockHash,CTR_SHA_256);	
		
				
		// Checking each block in current IVFC level
		for(int j = 0; j < blocknum; j++){
			//printf("  Checking Block %d\n",j);
			
			// Calculating Actual Hash of current block
			u64 BlockReadOffset = offset + (j*blocksize);
			u8 *BlockLocation = outbuff + BlockReadOffset;
			
			if(j == blocknum - 1){ 
				u64 BlockReadSize = size - (blocksize*(blocknum-1));
				memset(IVFC_Block,0,blocksize);
				memcpy(IVFC_Block,BlockLocation,BlockReadSize);
				BlockLocation = IVFC_Block;
			}
			
			u8 *WriteHashOffset = outbuff + prev_offset + (j*0x20);
			if(i==level1) WriteHashOffset = ctx->IVFC_MASTER_HASH.buffer + (j*0x20);
			
			ctr_sha(BlockLocation,blocksize,WriteHashOffset,CTR_SHA_256);
			if(memcmp(WriteHashOffset,EmptyBlockHash,0x20) == 0) memset(WriteHashOffset,0,0x20);
		}
		
		// Freeing block
		free(IVFC_Block);
	}
	
	// Duplicating IVFC hash tree
	memcpy(outbuff+IVFC_FILE_OFFSET+ctx->TableSize[tableivfc],outbuff+IVFC_FILE_OFFSET,ctx->TableSize[tableivfc]);
	
	return 0;
}

int GenerateDIFIPartitions(CreateExtdataCTX *ctx, u8 *outbuff)
{
	DIFI_PARTITION partition[2];
	memset(&partition,0,sizeof(DIFI_PARTITION)*2);
	for(u32 i = 0; i < 2; i++){
		GenerateDIFIPartition(i,&partition[i],ctx);
		memcpy(outbuff+ctx->PARTITION_OFFSET[i],&partition[i],sizeof(DIFI_PARTITION));
		memcpy(outbuff+ctx->PARTITION_OFFSET[i]+sizeof(DIFI_PARTITION),ctx->IVFC_MASTER_HASH.buffer,ctx->IVFC_MASTER_HASH.size);
	}
	free(ctx->IVFC_MASTER_HASH.buffer);
	ctr_sha(outbuff+ctx->PARTITION_OFFSET[ctx->ActiveDIFI],ctx->PARTITION_SIZE,ctx->ActiveDIFIHash,CTR_SHA_256);
	
	return 0;
}

int GenerateDIFIPartition(u32 partition, DIFI_PARTITION *part, CreateExtdataCTX *ctx)
{
	//DIFI Header
	u32_to_u8(part->DIFI.magic_0,DIFI_MAGIC_0,BE);
	u32_to_u8(part->DIFI.magic_1,DIFI_MAGIC_1,BE);
	
	u64 ivfc_offset = sizeof(DIFI_STRUCT);
	u64 dpfs_offset = ivfc_offset + sizeof(IVFC_STRUCT);
	u64 hash_offset = dpfs_offset + sizeof(DPFS_STRUCT);
	
	u64_to_u8(part->DIFI.ivfc_blob_offset,ivfc_offset,LE);
	u64_to_u8(part->DIFI.dpfs_blob_offset,dpfs_offset,LE);
	u64_to_u8(part->DIFI.hash_blob_offset,hash_offset,LE);
	u64_to_u8(part->DIFI.ivfc_blob_size,sizeof(IVFC_STRUCT),LE);
	u64_to_u8(part->DIFI.dpfs_blob_size,sizeof(DPFS_STRUCT),LE);
	u64_to_u8(part->DIFI.hash_blob_size,ctx->IVFC_MASTER_HASH.size,LE);
	u64_to_u8(part->DIFI.data_partition_offset,ctx->DATA_OFFSET,LE);
	
	if(partition == Primary) part->DIFI.flags[1] = 1; 
	if(ctx->ExtdataType == BUILD_DATA) part->DIFI.flags[0] = 1;
	
	// IVFC Blob
	u32_to_u8(part->IVFC.magic_0,IVFC_MAGIC_0,BE);
	u32_to_u8(part->IVFC.magic_1,IVFC_MAGIC_1,BE);
	u64_to_u8(part->IVFC.master_hash_size,ctx->IVFC_MASTER_HASH.size,LE);
	for(int i = level1; i <= level4; i++){
		u64_to_u8(part->IVFC.level[i].relative_offset,ctx->IVFC_OFFSET[i],LE);
		u64_to_u8(part->IVFC.level[i].size,ctx->IVFC_SIZE[i],LE);
		u64_to_u8(part->IVFC.level[i].block_size,log2l(ctx->IVFC_BLK_SIZE[i]),LE);
	}
	u64_to_u8(part->IVFC.unknown_0,0x78,LE);
	
	// DPFS Blob
	u32_to_u8(part->DPFS.magic_0,DPFS_MAGIC_0,BE);
	u32_to_u8(part->DPFS.magic_1,DPFS_MAGIC_1,BE);
	for(int i = 0; i < 3; i++){
		u64_to_u8(part->DPFS.table[i].offset,ctx->TableOffset[i],LE);
		u64_to_u8(part->DPFS.table[i].size,ctx->TableSize[i],LE);
		u64_to_u8(part->DPFS.table[i].block_size,log2l(ctx->TableBLKSize[i]),LE);
	}
	
	return 0;
}

int GenerateDIFFHeader(CreateExtdataCTX *ctx, u8 *outbuff)
{
	DIFF_STRUCT diff;
	memset(&diff,0,sizeof(DIFF_STRUCT));
	
	u32_to_u8(diff.magic_0,DIFF_MAGIC_0,BE);
	u32_to_u8(diff.magic_1,DIFF_MAGIC_1,BE);
	u64_to_u8(diff.secondary_partition_offset,ctx->PARTITION_OFFSET[Secondary],LE);
	u64_to_u8(diff.primary_partition_offset,ctx->PARTITION_OFFSET[Primary],LE);
	u64_to_u8(diff.partition_table_length,ctx->PARTITION_SIZE,LE);
	u64_to_u8(diff.file_base_offset,ctx->FB_OFFSET,LE);
	u64_to_u8(diff.file_base_size,ctx->FB_SIZE,LE);
	u32_to_u8(diff.active_partition,ctx->ActiveDIFI,LE);
	memcpy(diff.active_partition_hash,ctx->ActiveDIFIHash,0x20);
	memcpy(diff.extdata_unique_id,ctx->UniqueExtdataID,8);
	
	memcpy(outbuff+0x100,&diff,0x100);
	return 0;
}

void FreeExtdataContext(ExtdataContext *ctx)
{
	if(ctx->Files.Count > 0)
		_free(ctx->Files.Data);
	if(ctx->extdata.size > 0)
		_free(ctx->extdata.buffer);
	_free(ctx);
}

int UpdateExtdataHashTree(ExtdataContext *ctx)
{
	return 0;
}
