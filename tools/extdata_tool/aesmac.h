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
typedef enum
{
	mac_import_db = 1,
	mac_title_db,
	mac_extdata,
	mac_sys_save,
	mac_sd_save,
	mac_card_save
} aes_mac_types;

typedef enum
{
	CTR_EXT0 = 0,
	CTR_SYS0,
	CTR_NOR0,
	CTR_SAV0,
	CTR_SIGN,
	CTR_9DB0,
} savegame_type_index;

typedef struct
{
	u8 savegame_type[0x8];
	u8 image_id[4];
	u8 subdir_id[4];
	u8 unknown[4];
	u8 image_id_dup[4];
	u8 subdir_id_dup[4];
	u8 diff_header[0x100];
} extdata_sha256_block;

typedef struct
{
	u8 savegame_type[0x8];
	u8 save_id[0x8];
	u8 disa_header[0x100];
} sys_savedata_sha256_block;

typedef struct
{
	u8 savegame_type[0x8];
	u8 disa_header[0x100];
} ctr_nor0_sha256_block;

typedef struct
{
	u8 savegame_type[0x8];
	u8 disa_header[0x100];
} ctr_sav0_sha256_block;

typedef struct
{
	u8 savegame_type[0x8];
	u8 ctr_nor0_block_hash[0x20];
} gamecard_savegame_sha256_block;

typedef struct
{
	u8 savegame_type[0x8];
	u8 program_id[0x8];
	u8 ctr_sav0_block_hash[0x20];
} sdcard_savegame_sha256_block;

typedef struct
{
	u8 savegame_type[0x8];
	u8 db_id[0x4];
	u8 diff_header[0x100];
} db_sha256_block;

typedef struct
{
	// Input
	u8 header[0x100]; // DISA/DIFF header
	u8 type;
	u8 Verbose;
	
	//ExtData ONLY Input
	u8 image_id[4];
	u8 subdir_id[4];
	u8 is_quote_dat;
	
	// Sys Savedata ONLY input
	u8 saveid[8];
	
	// SD Savedata ONLY input
	u8 programid[8];
	
	// Output
	u8 hash[0x20];
	u8 aesmac[0x10];
} AESMAC_CTX;

int PrepAESMAC(AESMAC_CTX *ctx);
int GenAESMAC(u8 KeyX[16], u8 KeyY[16], AESMAC_CTX *ctx);
