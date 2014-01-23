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
#include "aesmac.h"
#include "ctr_crypto.h"


int PrepAESMAC(AESMAC_CTX *ctx)
{
	u8 savegame_type[6][8] = {"CTR-EXT0","CTR-SYS0","CTR-NOR0","CTR-SAV0","CTR-SIGN","CTR-9DB0"};
	memset(ctx->hash,0,0x20);
	
	if(ctx->type == 0)
		return Fail;
	else if(ctx->type == mac_import_db){
		db_sha256_block finalblock;
		memset(&finalblock,0,sizeof(db_sha256_block));
		memcpy(finalblock.savegame_type,savegame_type[CTR_9DB0],8);
		u32_to_u8(finalblock.db_id,0x3,LE);
		memcpy(finalblock.diff_header,ctx->header,0x100);
		ctr_sha(&finalblock,sizeof(db_sha256_block),ctx->hash,CTR_SHA_256);
	}
	else if(ctx->type == mac_title_db){
		db_sha256_block finalblock;
		memset(&finalblock,0,sizeof(db_sha256_block));
		memcpy(finalblock.savegame_type,savegame_type[CTR_9DB0],8);
		u32_to_u8(finalblock.db_id,0x2,LE);
		memcpy(finalblock.diff_header,ctx->header,0x100);
		ctr_sha(&finalblock,sizeof(db_sha256_block),ctx->hash,CTR_SHA_256);
	}
	else if(ctx->type == mac_extdata){
		extdata_sha256_block finalblock;
		memset(&finalblock,0,sizeof(extdata_sha256_block));
		memcpy(finalblock.savegame_type,savegame_type[CTR_EXT0],8);
		endian_memcpy(finalblock.image_id,ctx->image_id,4,LE);
		endian_memcpy(finalblock.subdir_id,ctx->subdir_id,4,LE);
		u32_to_u8(finalblock.unknown,ctx->is_quote_dat,LE);
		endian_memcpy(finalblock.image_id_dup,ctx->image_id,4,LE);
		endian_memcpy(finalblock.subdir_id_dup,ctx->subdir_id,4,LE);
		memcpy(finalblock.diff_header,ctx->header,0x100);
		ctr_sha(&finalblock,sizeof(extdata_sha256_block),ctx->hash,CTR_SHA_256);
	}
	else if(ctx->type == mac_sys_save){
		sys_savedata_sha256_block finalblock;
		memset(&finalblock,0,sizeof(sys_savedata_sha256_block));
		memcpy(finalblock.savegame_type,savegame_type[CTR_SYS0],8);
		endian_memcpy(finalblock.save_id,ctx->saveid,8,LE);
		memcpy(finalblock.disa_header,ctx->header,0x100);
		ctr_sha(&finalblock,sizeof(sys_savedata_sha256_block),ctx->hash,CTR_SHA_256);
	}
	else if(ctx->type == mac_sd_save){
		sdcard_savegame_sha256_block finalblock;
		memset(&finalblock,0,sizeof(sdcard_savegame_sha256_block));
		memcpy(finalblock.savegame_type,savegame_type[CTR_SIGN],8);
		endian_memcpy(finalblock.program_id,ctx->programid,8,LE);
		// Generating CTR_SAV0 hash
		ctr_sav0_sha256_block pre_block;
		memset(&pre_block,0,sizeof(ctr_sav0_sha256_block));
		memcpy(pre_block.savegame_type,savegame_type[CTR_SAV0],8);
		memcpy(pre_block.disa_header,ctx->header,0x100);
		ctr_sha(&pre_block,sizeof(ctr_sav0_sha256_block),finalblock.ctr_sav0_block_hash,CTR_SHA_256);
		//
		ctr_sha(&finalblock,sizeof(sdcard_savegame_sha256_block),ctx->hash,CTR_SHA_256);
	}
	else if(ctx->type == mac_card_save){
		gamecard_savegame_sha256_block finalblock;
		memset(&finalblock,0,sizeof(gamecard_savegame_sha256_block));
		memcpy(finalblock.savegame_type,savegame_type[CTR_SAV0],8);
		// Generating CTR_NOR0 hash
		ctr_nor0_sha256_block pre_block;
		memset(&pre_block,0,sizeof(ctr_nor0_sha256_block));
		memcpy(pre_block.savegame_type,savegame_type[CTR_NOR0],8);
		memcpy(pre_block.disa_header,ctx->header,0x100);
		ctr_sha(&pre_block,sizeof(ctr_nor0_sha256_block),finalblock.ctr_nor0_block_hash,CTR_SHA_256);
		//
		ctr_sha(&finalblock,sizeof(gamecard_savegame_sha256_block),ctx->hash,CTR_SHA_256);
	}
	else
		return Fail;
	
	return 0;
}

int GenAESMAC(u8 KeyX[16], u8 KeyY[16], AESMAC_CTX *ctx)
{
	/* Generating Hash for AES MAC */
	u8 result = PrepAESMAC(ctx);
	if(result)
		return result;
	
	/* Preparing for Crypto functions */
	u8 *encrypted_hash = malloc(0x20);
	u8 *outkey = malloc(16);
	u8 IV[16];
	memset(encrypted_hash,0,0x20);
	memset(outkey,0,16);
	memset(&IV,0,16);
	
	/* Getting Key for AES MAC */
	ctr_keyscrambler(outkey,KeyX,KeyY); //Not implemented yet, as no one knows that. ATM, it's a placeholder keyscrambler for testing.
	
	/* Crypting hash */
	ctr_aes_context aes_cbc;
	ctr_init_aes_cbc(&aes_cbc,outkey,IV,ENC);
	ctr_aes_cbc(&aes_cbc,ctx->hash,encrypted_hash,0x20,ENC);
	
	if(ctx->Verbose){
		memdump(stdout," KeyX:              ",KeyX,16);
		memdump(stdout," KeyY:              ",KeyY,16);
		memdump(stdout," Normal Key:        ",outkey,16);
		memdump(stdout," Hash For MAC:      ",ctx->hash,0x20);
		memdump(stdout," MAC:               ",encrypted_hash+0x10,16);
	}
	
	
	/* Writing AES MAC to context */
	memcpy(ctx->aesmac,encrypted_hash+0x10,16);	

	free(outkey);
	free(encrypted_hash);
	return 0;
}
