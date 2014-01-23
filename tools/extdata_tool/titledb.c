/**
Copyright 2013 3DSGuy

This file is part of extdata_tool.

extdata_tool is free software: you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation, either version 3 of the License, or
(at your option) any later version.

extdata_tool is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with extdata_tool.  If not, see <http://www.gnu.org/licenses/>.
**/
#include "lib.h"
#include "titledb.h"

const char DB_MAGIC[4][8] = {"NANDTDB","NANDIDB","TEMPTDB","TEMPIDB"};
const u8 BDRI_MAGIC[5] = {"BDRI"};
const char TitlePlatformString[2][5] = {"CTR","TWL"};
const char TitleTypeString[2][8] = {"","System "};
const char TitleAppTypeString[9][20] = {"Application","DLP Child","Demo","Add-on Content","DLC Content","Applet","Module","Data Archive","Firmware"};

static DATABASE_CONTEXT db_ctx;

// Private Prototypes
void GetDB_Type(char *db_type_magic);
void ProcessDB_Header(void);
int GetEntry_Header(void);
void PopulateDatabase(void);
void PrintDatabase(void);
void PrintTitleData(TITLE_CONTEXT *TitleData);
int EntryUsed(TITLE_CONTEXT *TitleData);
int EntryValid(TITLE_CONTEXT *TitleData);
void PrintTitleIndexData(TITLE_INDEX_ENTRY_STRUCT *index);
void PrintTitleInfoData(TITLE_INFO_ENTRY_STRUCT *info);
void GetTitleType(u8 TitleID[8]);
void ListDatabase(void);
u32 GetValidEntryCount(void);
void CollectTitleIDs(u64 *TitleID_DB, u32 ContentCount);
void SortTitleIDs(u64 *TitleID_DB, u32 ContentCount);
void ListTitleIDs(u64 *TitleID_DB, u32 ContentCount);

// Code
int IsTitleDB(u8 *db)
{
	BDRI_STRUCT *bdri = (BDRI_STRUCT*)(db+0x80);
	if(memcmp(bdri->magic_0,BDRI_MAGIC,4) == 0 && u8_to_u32(bdri->magic_1,BE) == 0x300)
		return True;
	return False;
}

int ProcessTitleDB(u8 *db, int Mode)
{
	memset(&db_ctx,0x0,sizeof(DATABASE_CONTEXT));
	db_ctx.db = (db + 0x80);	
	GetDB_Type((char*)db);
	ProcessDB_Header();
	
	if(GetEntry_Header() != ValidDB){
		printf("[!] Error\n");
		return CorruptDB;
	}

	PopulateDatabase();

	if(Mode == DB_Normal)
		PrintDatabase();
	if(Mode == DB_ByTID)
		ListDatabase();

	if(db_ctx.database.BufferAllocated == True){
		free(db_ctx.database.TitleData);
	}
	return 0;
}

void GetDB_Type(char *db_type_magic)
{
	for(int i = 0; i < 4; i++){
		if(memcmp(DB_MAGIC[i],db_type_magic,7) == 0){
			db_ctx.core_data.db_type = i + 1;
			return;
		}
	}
	db_ctx.core_data.db_type = Invalid;
}

void ProcessDB_Header(void)
{
	db_ctx.header = (BDRI_STRUCT*)db_ctx.db;
	db_ctx.core_data.EntryTableOffset = u8_to_u64(db_ctx.header->Entry_Table_Offset,LE);
}

int GetEntry_Header(void)
{
	db_ctx.entry_table_header = (ENTRY_TABLE_HEADER*)(db_ctx.db + db_ctx.core_data.EntryTableOffset);
	if(u8_to_u32(db_ctx.entry_table_header->magic_0,LE) != 0x2 || u8_to_u32(db_ctx.entry_table_header->magic_1,LE) != 0x3 ){
		printf("[!] Embedded Title Entry Table section is Missing or Corrupt\n");
		return CorruptDB;
	}
	db_ctx.database.TitleCount = u8_to_u32(db_ctx.entry_table_header->entry_count,LE);
	db_ctx.database.MaxCount = u8_to_u32(db_ctx.entry_table_header->max_entry_count,LE);
	return ValidDB;
}

void PopulateDatabase(void)
{
	db_ctx.database.BufferAllocated = True;
	db_ctx.database.TitleData = malloc(sizeof(TITLE_CONTEXT)*db_ctx.database.MaxCount);
	memset(db_ctx.database.TitleData,0x0,(sizeof(TITLE_CONTEXT)*db_ctx.database.MaxCount));
	
	for(u32 i = 0; i < db_ctx.database.MaxCount; i++){
		u32 entry_offset = (db_ctx.core_data.EntryTableOffset + sizeof(ENTRY_TABLE_HEADER) + (i * sizeof(TITLE_INDEX_ENTRY_STRUCT)));
		TITLE_INDEX_ENTRY_STRUCT *temp = (TITLE_INDEX_ENTRY_STRUCT*)(db_ctx.db+entry_offset);
		if(u8_to_u32(temp->Active_Entry,LE) == True){
			u32 Index = u8_to_u32(temp->Index,LE);
			u32 info_offset = (db_ctx.core_data.EntryTableOffset + (u8_to_u32(temp->Title_Info_Offset,LE) * u8_to_u32(temp->Title_Info_Offset_Media,LE)));
			db_ctx.database.TitleData[Index].index = temp;
			db_ctx.database.TitleData[Index].info = (TITLE_INFO_ENTRY_STRUCT*)(db_ctx.db+info_offset);
		}
	}
} 

void PrintDatabase(void)
{
	printf("[+] Database Info:\n");
	printf(" Title Database Type: ");
	switch(db_ctx.core_data.db_type){
		case NANDTDB : printf("NAND Title Database \n"); break;
		case NANDIDB : printf("NAND Title Import Database \n"); break;
		case TEMPTDB : printf("SDMC Title Database \n"); break;
		case TEMPIDB : printf("NAND DLP Child Temporary Database \n"); break;
		default : printf("Unknown\n"); break;
	}	
	printf(" Active Entries:      %d\n",GetValidEntryCount());
	printf(" Maximum Entries:     %d\n",db_ctx.database.MaxCount);
	printf("[+] Titles Entries in Database:\n");
	for(u32 i = 0; i < db_ctx.database.MaxCount; i++){
		PrintTitleData(&db_ctx.database.TitleData[i]);
	}
}

void PrintTitleData(TITLE_CONTEXT *TitleData)
{
	if(EntryUsed(TitleData)){
		printf(" [+]: Unused\n");
		return;
	}
	if(EntryValid(TitleData)){
		printf(" [+]: Invalid\n");
		return;
	}
	PrintTitleIndexData(TitleData->index);
	PrintTitleInfoData(TitleData->info);
}

int EntryUsed(TITLE_CONTEXT *TitleData)
{
	if(TitleData->index == NULL)
		return Invalid;
	if(u8_to_u32(TitleData->index->Active_Entry,LE) != True)
		return Invalid;
	return Valid;
}

int EntryValid(TITLE_CONTEXT *TitleData)
{
	if(TitleData->info == NULL)
		return Invalid;
	if(u8_to_u32(TitleData->info->Title_Type,LE) == 0x0)
		return Invalid;
	return Valid;
}

void PrintTitleIndexData(TITLE_INDEX_ENTRY_STRUCT *index)
{
	printf(" [+]: %d\n",u8_to_u32(index->Index,LE));
	printf(" TitleID:                    "); u8_hex_print_le(index->Title_ID, 8); printf("\n");
	GetTitleType(index->Title_ID);
}

void PrintTitleInfoData(TITLE_INFO_ENTRY_STRUCT *info)
{
	printf(" Product Code:               %.16s\n",info->Product_Code);
	printf(" Title Type:                 %x\n",u8_to_u32(info->Title_Type,LE));
	printf(" Title Version:              v%d\n",u8_to_u16(info->Title_Version,LE));
	printf(" TMD Content ID:             %08x\n",u8_to_u32(info->TMD_File_ID,LE));
	printf(" CMD Content ID:             %08x\n",u8_to_u32(info->CMD_File_ID,LE));
	printf(" ExtdataID low:              %08x\n",u8_to_u32(info->ExtData_ID,LE));
	printf(" Manual:                     %s\n",info->Flags_0[0]? "YES" : "NO");
	printf(" SD Save Data:               %s\n",info->Flags_1[0]? "YES" : "NO");
	printf(" Is DSiWare:                 %s\n",info->Flags_2[0]? "YES" : "NO");
	/**
	printf("  Flags_0:                   "); u8_hex_print_be(info->Flags_0,0x4);putchar('\n');
	//printf("   > Manual:                 %s\n",info->Flags_0[0]? "YES" : "NO");
	printf("   > UNK_1:                    %s\n",info->Flags_0[1]? "YES" : "NO");
	printf("   > UNK_2:                    %s\n",info->Flags_0[2]? "YES" : "NO");
	printf("   > UNK_3:                    %s\n",info->Flags_0[3]? "YES" : "NO");
	//printf("  TMD Content ID:            %08x\n",info->TMD_File_ID);
	//printf("  CMD Content ID:            %08x\n",info->CMD_File_ID);
	printf("  Flags_1:                   "); u8_hex_print_be(info->Flags_1,0x4);putchar('\n');
	//printf("   > SD Save Data:           %s\n",info->Flags_1[0]? "YES" : "NO");
	printf("   > UNK_1:                    %s\n",info->Flags_1[1]? "YES" : "NO");
	printf("   > UNK_2:                    %s\n",info->Flags_1[2]? "YES" : "NO");
	printf("   > UNK_3:                    %s\n",info->Flags_1[3]? "YES" : "NO");
	printf("  ExtdataID low:             %08x\n",info->ExtData_ID);
	printf("  Flags_2:                   "); u8_hex_print_be(info->Flags_2,0x8);putchar('\n');
	//printf("   > DSiWare:                %s\n",info->Flags_2[0]? "YES" : "NO");//DSiWare Related, Export Flag?
	printf("   > UNK_1:                    %s\n",info->Flags_2[1]? "YES" : "NO");
	printf("   > UNK_2:                    %s\n",info->Flags_2[2]? "YES" : "NO");
	printf("   > UNK_3:                    %s\n",info->Flags_2[3]? "YES" : "NO");
	//printf("   > Type:           %s\n",info->Flags_2[4]? "Regular" : "System");
	//printf("   > DSiWare Related:        %s\n",info->Flags_2[5]? "YES" : "NO");//DSiWare Related, Export Flag?
	printf("   > UNK_6:                    %s\n",info->Flags_2[6]? "YES" : "NO");
	printf("   > UNK_7:                    %s\n",info->Flags_2[7]? "YES" : "NO");
	**/
	//printf(" '0xAC' Increment:           %08x\n",info->unknown_6);
}

void GetTitleType(u8 TitleID[8])
{
	//u8 FlagBool[16];
	u16 TitleTypeID = u8_to_u16(TitleID+4,LE);
	//resolve_flag_u16(TitleTypeID,FlagBool);
	
	int TitlePlatform_FLAG = CTR;
	int TitleType_FLAG = Regular;
	int TitleAppType_FLAG = Application;
	
	if(CheckBitmask(TitleTypeID,0x8000) && !CheckBitmask(TitleTypeID,0x4000)){
		TitlePlatform_FLAG = TWL;
		if(CheckBitmask(TitleTypeID,0x1)){
			TitleType_FLAG = System;
			printf("Hey\n");
		}
		else{
			TitleType_FLAG = Regular;
		}
		
		if(CheckBitmask(TitleTypeID,0x4)){
			TitleAppType_FLAG = Application;
			if(CheckBitmask(TitleTypeID,0x2)){
				TitleAppType_FLAG = Applet;
				if(CheckBitmask(TitleTypeID,0x8)){
					TitleAppType_FLAG = Data_Archive;
				}
			}
		}
	}
	
	else if(TitlePlatform_FLAG == CTR){
		if(CheckBitmask(TitleTypeID,0x10) && !CheckBitmask(TitleTypeID,0x4000)){
			TitleType_FLAG = System;
			
		}
		else{
			TitleType_FLAG = Regular;
		}
		if(CheckBitmask(TitleTypeID,0x1)) TitleAppType_FLAG = DLP_Child;
		if(CheckBitmask(TitleTypeID,0x2)) TitleAppType_FLAG = Demo;
		if(CheckBitmask(TitleTypeID,0xB)) TitleAppType_FLAG = Data_Archive;
		if(CheckBitmask(TitleTypeID,0xE)) TitleAppType_FLAG = Addon_Content;
		if(CheckBitmask(TitleTypeID,0x8C)) TitleAppType_FLAG = DLC_Content;
		if(CheckBitmask(TitleTypeID,0x20)){
			TitleAppType_FLAG = Applet;
			if(CheckBitmask(TitleTypeID,0x100)){
				TitleAppType_FLAG = Module;
				if(CheckBitmask(TitleTypeID,0x8)){
					TitleAppType_FLAG = Firmware;
				}
			}
		}
		
	}

	printf(" Platform:                   %s\n",TitlePlatformString[TitlePlatform_FLAG]);
	printf(" Content Type:               %s%s\n",TitleTypeString[TitleType_FLAG],TitleAppTypeString[TitleAppType_FLAG]);
	if(TitlePlatform_FLAG == TWL){
		printf(" Region Lock:                ");
		switch(TitleID[0]){
			case 0x41 : printf("Region Free\n"); break;
			case 0x45 : printf("America\n"); break;
			case 0x4A : printf("Japan\n"); break;
			case 0x56 : printf("PAL\n"); break;
		}
	}
	if(TitlePlatform_FLAG == CTR && TitleType_FLAG == System && TitleAppType_FLAG != Application){
		printf(" Kernel:                     ");
		switch(TitleID[0]){
			case 0x2 : printf("NATIVE_FIRM\n"); break;
			case 0x3 : printf("SAFE_MODE_FIRM\n"); break;
		}
	}
}

void ListDatabase(void)
{
	u32 ContentCount = GetValidEntryCount();
	u64 len = sizeof(u64) * ContentCount;
	u64 TitleID_DB[len];
	memset(&TitleID_DB,0x0,(sizeof(u64)*ContentCount));
	CollectTitleIDs(TitleID_DB,ContentCount);
	merge_sort(TitleID_DB,ContentCount);
	ListTitleIDs(TitleID_DB,ContentCount);
}

u32 GetValidEntryCount(void)
{
	u32 counter = 0;
	for(u32 i = 0; i < db_ctx.database.MaxCount; i++){
		if(EntryUsed(&db_ctx.database.TitleData[i]) == Valid && EntryValid(&db_ctx.database.TitleData[i]) == Valid)
			counter++;
	}
	return counter;
}

void CollectTitleIDs(u64 *TitleID_DB, u32 ContentCount)
{
	for(u32 i = 0, TID_Count = 0; i < db_ctx.database.MaxCount || TID_Count < ContentCount; i++){
		if(EntryUsed(&db_ctx.database.TitleData[i]) == Valid){
			TitleID_DB[TID_Count] = u8_to_u64(db_ctx.database.TitleData[i].index->Title_ID,LE);
			TID_Count++;
		}
	}
}

void ListTitleIDs(u64 *TitleID_DB, u32 ContentCount)
{
	printf("[+] Title Count: %d\n",ContentCount);
	printf("[+] Title List:\n");
	for(u32 i = 0; i < ContentCount; i++){
		printf(" %016llx\n",TitleID_DB[i]);
	}
}
