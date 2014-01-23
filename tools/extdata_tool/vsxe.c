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
#include "vsxe.h"

static VSXE_INTERNAL_CONTEXT vsxe_ctx;

// Private Prototypes
// - Preparation Functions
int VSXE_SetupInternalContext(VSXEContext *ctx);
void VSXE_InterpreteFolderTable(void);
void VSXE_InterpreteFileTable(void);

// - Info Functions
void VSXE_PrintFSInfo(void);
void VSXE_PrintDir(u32 dir, char *pre_string, char *indent);
void VSXE_PrintFiles_In_Dir(u32 dir, char *pre_string, char *indent);
u32 VSXE_GetDir_FileCount(u32 dir);
u32 VSXE_GetDir_FolderCount(u32 dir);
u32 VSXE_GetLevel(u32 dir);

// - Extracting Functions
int VSXE_Extract_FileSystem(u32 dir);
int VSXE_ExportExtdataImagetoFile(char *inpath, FILE *out, u8 *ExtdataUniqueID);
void VSXE_ReturnDirPath(u32 folder_id, char *path, u8 platform); // Retained, but not used
void VSXE_ReturnExtdataMountPath(u32 file_id, char *path, u8 platform); // Retained, but not used

// Code
int IsVSXEFileSystem(u8 *vsxe)
{
	vsxe_header *header = (vsxe_header*)vsxe;
	
	if(u8_to_u32(header->magic,BE) == vsxe_magic && u8_to_u32(header->magic_id,LE) == vsxe_magic_id){
		return True;
	}
	
	return False;
}

int ProcessExtData_FS(VSXEContext *ctx)
{
	memset(&vsxe_ctx,0,sizeof(VSXE_INTERNAL_CONTEXT));
	
	u8 result = VSXE_SetupInternalContext(ctx);
	if(result) return result;
	
	VSXE_PrintFSInfo();
	
	if(ctx->Flags[vsxe_extract] == True){
		if(VSXE_Extract_FileSystem(1) != 0){
			printf("[!] Failed to Extract ExtData images\n");
			return 1;
		}
	}
	return 0;
}

int VSXE_SetupInternalContext(VSXEContext *ctx)
{
	vsxe_ctx.vsxe = ctx->vsxe;
	vsxe_ctx.input = ctx->input;
	vsxe_ctx.output = ctx->output;
	vsxe_ctx.platform = ctx->platform;
	vsxe_ctx.verbose = ctx->Flags[vsxe_verbose];
	vsxe_ctx.showfs = ctx->Flags[vsxe_show_fs];
	vsxe_ctx.showfs_tables = ctx->Flags[vsxe_fstable];	

	vsxe_ctx.header = (vsxe_header*)vsxe_ctx.vsxe;
	vsxe_ctx.data_table = (vsxe_data_table*)(vsxe_ctx.vsxe + u8_to_u64(vsxe_ctx.header->data_table_offset,LE));
	
	if(u8_to_u32(vsxe_ctx.header->magic,BE) != vsxe_magic || u8_to_u32(vsxe_ctx.header->magic_id,LE) != vsxe_magic_id){
		printf("[!] Is not a VSXE File Table\n");
		return 1;
	}
	
	
	vsxe_ctx.folder_table_offset = u8_to_u64(vsxe_ctx.data_table->folder_table_offset,LE);
	folder_table_header *header = (folder_table_header*)(vsxe_ctx.vsxe + vsxe_ctx.folder_table_offset);
	u64 folderTableMaxSize = (((u8_to_u32(header->max_slots,LE))*sizeof(folder_entry)));
	vsxe_ctx.file_table_offset = align_value(vsxe_ctx.folder_table_offset+folderTableMaxSize,0x1000);
	
	if(vsxe_ctx.verbose){
		printf("[+] Folder Table Offset:    0x%llx\n",vsxe_ctx.folder_table_offset);
		printf("[+] File Table Offset:      0x%llx\n",vsxe_ctx.file_table_offset);
	}

	VSXE_InterpreteFolderTable();
	VSXE_InterpreteFileTable();
	
	return 0;
}

void VSXE_InterpreteFolderTable(void)
{
	folder_table_header *header = (folder_table_header*)(vsxe_ctx.vsxe + vsxe_ctx.folder_table_offset);
	vsxe_ctx.foldercount = u8_to_u32(header->used_slots,LE);
	vsxe_ctx.folders = (folder_entry*)(vsxe_ctx.vsxe + vsxe_ctx.folder_table_offset);

	if(vsxe_ctx.verbose){
		printf("[+] Maximum Folder Entries: %d\n",u8_to_u32(header->max_slots,LE)-1);
		printf("[+] Used Folder Entries:    %d\n",u8_to_u32(header->used_slots,LE)-1);
	}
	return;
}

void VSXE_InterpreteFileTable(void)
{
	file_table_header *header = (file_table_header*)(vsxe_ctx.vsxe + vsxe_ctx.file_table_offset);
	vsxe_ctx.filecount = u8_to_u32(header->used_slots,LE);
	vsxe_ctx.files = (file_entry*)(vsxe_ctx.vsxe + vsxe_ctx.file_table_offset);

	if(vsxe_ctx.verbose){
		printf("[+] Maximum File Entries:   %d\n",u8_to_u32(header->max_slots,LE)-1);
		printf("[+] Used File Entries:      %d\n",u8_to_u32(header->used_slots,LE)-1);
	}
	return;
}

void VSXE_PrintFSInfo(void)
{
	if(vsxe_ctx.showfs){
		printf("Last used Extdata Details:\n");
		printf(" ExtData Image ID:     %08x\n",u8_to_u32(vsxe_ctx.header->last_used_file_extdata_id,LE));
		printf(" ExtData Mount Path:   %s\n",vsxe_ctx.header->last_used_file);
		/**
		printf(" ExtData Action:       ");
		switch(u8_to_u32(vsxe_ctx.header->last_used_file_action,LE)){
			case vsxe_fs_deleted: printf("Deleted or Unique Extdata ID was blanked\n"); break;
			case vsxe_fs_modified: printf("Modified\n"); break;
		}
		**/
		
		printf("ExtData FileSystem and Mount Locations:\n");
		printf(" root\n");
		VSXE_PrintDir(1," ",NULL);
	}		
	if(vsxe_ctx.showfs_tables){
		printf("\nFolders\n");
		for(int i = 1; i < vsxe_ctx.foldercount; i++){
			u32 ParentFolderIndex = u8_to_u32(vsxe_ctx.folders[i].parent_folder_index,LE);
			u32 PreviousFolderIndex = u8_to_u32(vsxe_ctx.folders[i].prev_folder_id,LE);
			u32 LastFolderIndex = u8_to_u32(vsxe_ctx.folders[i].last_folder_index,LE);
			u32 LastFileIndex = u8_to_u32(vsxe_ctx.folders[i].last_file_index,LE);
			printf("------------------------------------\n\n");
			printf("Current Folder Index:  %d\n",i);
			if(ParentFolderIndex > 1)
				printf("Parent Folder Index:   %d (%s)\n",ParentFolderIndex,vsxe_ctx.folders[ParentFolderIndex].filename);
			else if(ParentFolderIndex == 1)
				printf("Parent Folder Index:   %d (root)\n",ParentFolderIndex);
			else
				printf("Parent Folder Index:   %d\n",ParentFolderIndex);
			printf("Folder Name:           %s\n",vsxe_ctx.folders[i].filename);
			if(PreviousFolderIndex < 1)
				printf("Prev. Folder Index:    %d (Is First Folder in Dir)\n",PreviousFolderIndex);
			else if(PreviousFolderIndex == 1)
				printf("Prev. Folder Index:    %d (root)\n",PreviousFolderIndex);
			else
				printf("Prev. Folder Index:    %d (%s)\n",PreviousFolderIndex,vsxe_ctx.folders[PreviousFolderIndex].filename);
			if(LastFolderIndex)
				printf("Last Folder Index:     %d (%s)\n",LastFolderIndex,vsxe_ctx.folders[LastFolderIndex].filename);
			else
				printf("Last Folder Index:     %d\n",LastFolderIndex);
			if(LastFileIndex)
				printf("Last File Index:       %d (%s)\n",LastFileIndex,vsxe_ctx.files[LastFileIndex].filename);
			else
				printf("Last File Index:       %d\n",LastFileIndex);
				
			printf("UNK0:                  %d\n",u8_to_u32(vsxe_ctx.folders[i].unk0,LE));
			printf("UNK1:                  %d\n",u8_to_u32(vsxe_ctx.folders[i].unk1,LE));
			printf("\n------------------------------------\n");
		}
		
		printf("\nFiles\n");
		for(int i = 1; i < vsxe_ctx.filecount; i++){
			u32 ParentFolderIndex = u8_to_u32(vsxe_ctx.files[i].parent_folder_index,LE);
			u32 PreviousFileIndex = u8_to_u32(vsxe_ctx.files[i].prev_file_id,LE);
			printf("------------------------------------\n\n");
			printf("Current File Index:    %d\n",i);
			if(ParentFolderIndex == 1)
				printf("Parent Folder Index:   %d (root)\n",ParentFolderIndex);
			else
				printf("Parent Folder Index:   %d (%s)\n",ParentFolderIndex,vsxe_ctx.folders[ParentFolderIndex].filename);
			printf("File Name:             %s\n",vsxe_ctx.files[i].filename);
			if(PreviousFileIndex < 1)
				printf("Prev. File Index:      %d (Is First File in Dir)\n",PreviousFileIndex);
			else
				printf("Prev. File Index:      %d (%s)\n",PreviousFileIndex,vsxe_ctx.files[PreviousFileIndex].filename);
			printf("UNK0:                  %x\n",u8_to_u32(vsxe_ctx.files[i].unk0,LE));
			printf("UNK1:                  %x\n",u8_to_u32(vsxe_ctx.files[i].unk1,LE));
			memdump(stdout,"Unique Extdata ID:     ",vsxe_ctx.files[i].unique_extdata_id,8);
			printf("UNK2:                  %x\n",u8_to_u32(vsxe_ctx.files[i].unk2,LE));
			printf("UNK3:                  %x\n",u8_to_u32(vsxe_ctx.files[i].unk3,LE));
			printf("\n------------------------------------\n");
		}
	}	
}

void VSXE_PrintDir(u32 dir, char *pre_string, char *indent)
{
	u32 FolderCount = VSXE_GetDir_FolderCount(dir);
	u32 FileCount = VSXE_GetDir_FileCount(dir);
	
	if(FolderCount){
		u32 folders_index[FolderCount];
	
		u32 CurrentFolder = u8_to_u32(vsxe_ctx.folders[dir].last_folder_index,LE);
		for(s32 i = FolderCount-1; i >= 0; i--){
			folders_index[i] = CurrentFolder;
			CurrentFolder = u8_to_u32(vsxe_ctx.folders[CurrentFolder].prev_folder_id,LE);
		}
		
		for(u32 i = 0; i < FolderCount; i++){
			u32 index = folders_index[i];
			u32 CurrentFolder_FolderCount = VSXE_GetDir_FolderCount(index);
			u32 CurrentFolder_FileCount = VSXE_GetDir_FileCount(index);
			int HasNoChild = (!CurrentFolder_FileCount && !CurrentFolder_FolderCount);
			int IsLastChild = (!FileCount && i == FolderCount-1);
			
			char *filename = vsxe_ctx.folders[index].filename;
			if(pre_string!=NULL) printf("%s",pre_string);
			if(indent != NULL)
				printf("%s",indent);
			if(IsLastChild) printf("'-- ");
			else printf("|-- ");
			printf("%s\n",filename);
			
			if(!HasNoChild){
				char *tmp = malloc(IO_PATH_LEN);
				if(indent != NULL){
					if(!IsLastChild) sprintf(tmp,"%s|   ",indent);
					if(IsLastChild) sprintf(tmp,"%s    ",indent);
				}
				else{
					if(!IsLastChild) sprintf(tmp,"|   ");
					if(IsLastChild) sprintf(tmp,"    ");
				}
				VSXE_PrintDir(index,pre_string,tmp);
				free(tmp);
			}
		}
		
	}
	if(FileCount){
		VSXE_PrintFiles_In_Dir(dir,pre_string,indent);
	}
}

void VSXE_PrintFiles_In_Dir(u32 dir, char *pre_string, char *indent)
{
	u32 FileCount = VSXE_GetDir_FileCount(dir);
	
	if(!FileCount) return;
	
	u32 files_index[FileCount];
	
	u32 CurrentFile = u8_to_u32(vsxe_ctx.folders[dir].last_file_index,LE);
	for(int i = FileCount-1; i >= 0; i--){
		files_index[i] = CurrentFile;
		CurrentFile = u8_to_u32(vsxe_ctx.files[CurrentFile].prev_file_id,LE);
	}
		
	for(int i = 0; i < FileCount; i++){
		u32 index = files_index[i];
		char *filename = vsxe_ctx.files[index].filename;
		if(pre_string!=NULL) printf("%s",pre_string);
		if(indent != NULL)
			printf("%s",indent);
		if(i == FileCount-1) printf("'-- ");
			else printf("|-- ");
		printf("%s (%08x)\n",filename,index+1);
	}
		
	return;
	
}

u32 VSXE_GetDir_FileCount(u32 dir)
{
	u32 FileCount = 0;
	
	u32 CurrentFile = u8_to_u32(vsxe_ctx.folders[dir].last_file_index,LE);
	while (CurrentFile > 0){
		FileCount++;
		CurrentFile = u8_to_u32(vsxe_ctx.files[CurrentFile].prev_file_id,LE);
	}
	
	return FileCount;
}

u32 VSXE_GetDir_FolderCount(u32 dir)
{
	u32 FolderCount = 0;
	
	u32 CurrentFolder = u8_to_u32(vsxe_ctx.folders[dir].last_folder_index,LE);
	while (CurrentFolder > 0){
		FolderCount++;
		CurrentFolder = u8_to_u32(vsxe_ctx.folders[CurrentFolder].prev_folder_id,LE);
	}
	
	return FolderCount;
}

u32 VSXE_GetLevel(u32 dir)
{
	u32 path_part_count = 0;
	u32 present_dir = u8_to_u32(vsxe_ctx.folders[dir].parent_folder_index,LE);
	while(present_dir > 0){
		path_part_count++;
		present_dir = u8_to_u32(vsxe_ctx.folders[present_dir].parent_folder_index,LE);
	}
	return path_part_count;
}

int VSXE_Extract_FileSystem(u32 dir)
{
	if(dir == 1) chdir(vsxe_ctx.output);
	else chdir(vsxe_ctx.folders[dir].filename);
	
	int result = 0;
	u32 FolderCount = VSXE_GetDir_FolderCount(dir);
	u32 FileCount = VSXE_GetDir_FileCount(dir);
	
	if(FolderCount){
		u32 CurrentFolder = u8_to_u32(vsxe_ctx.folders[dir].last_folder_index,LE);
		for(u32 i = 0; i < FolderCount; i++){
			u32 CurrentFolder_FolderCount = VSXE_GetDir_FolderCount(CurrentFolder);
			u32 CurrentFolder_FileCount = VSXE_GetDir_FileCount(CurrentFolder);
			makedir(vsxe_ctx.folders[CurrentFolder].filename);
			if(CurrentFolder_FolderCount || CurrentFolder_FileCount){
				result = VSXE_Extract_FileSystem(CurrentFolder);
				if(result) return result;
			}
			CurrentFolder = u8_to_u32(vsxe_ctx.folders[CurrentFolder].prev_folder_id,LE);
		}
		
	}
	if(FileCount){
		u32 CurrentFile = u8_to_u32(vsxe_ctx.folders[dir].last_file_index,LE);
		char inpath[IO_PATH_LEN];
		FILE *outfile = NULL;
		for(u32 i = 0; i < FileCount; i++){
			outfile = fopen(vsxe_ctx.files[CurrentFile].filename,"wb");
			if(outfile == NULL){
				printf("[!] Failed to create '%s'\n",vsxe_ctx.files[CurrentFile].filename);
				return 1;
			}
			memset(inpath,0x0,IO_PATH_LEN);
			sprintf(inpath,"%s%c%08x.dec",vsxe_ctx.input,vsxe_ctx.platform,CurrentFile+1);
			if(vsxe_ctx.verbose) printf("[+] Writing data in '%s' to: '%s'\n",inpath,vsxe_ctx.files[CurrentFile].filename);
			if(VSXE_ExportExtdataImagetoFile(inpath,outfile,vsxe_ctx.files[CurrentFile].unique_extdata_id) != 0){
				printf("[!] Failed to Extract '%s' to '%s'\n",inpath,vsxe_ctx.files[CurrentFile].filename);
				return 1;
			}
			fclose(outfile);
			
			CurrentFile = u8_to_u32(vsxe_ctx.files[CurrentFile].prev_file_id,LE);
		}
	}
	
	if(dir > 1) chdir("..");
	
	return 0;
}

int VSXE_ExportExtdataImagetoFile(char *inpath, FILE *out, u8 *ExtdataUniqueID)
{
	ExtdataContext *ext = malloc(sizeof(ExtdataContext));
	InitaliseExtdataContext(ext);
	ext->extdata.size = GetFileSize_u64(inpath);
	ext->extdata.buffer = malloc(ext->extdata.size);
	memcpy(ext->VSXE_Extdata_ID,ExtdataUniqueID,8);
	if(ext->extdata.buffer == NULL) return VSXE_MEM_FAIL;
	FILE *infile = fopen(inpath,"rb");
	ReadFile_64(ext->extdata.buffer,ext->extdata.size,0,infile);
	fclose(infile);
	u8 result = GetExtdataContext(ext);
	if(result)
		return result;
	if(ext->ExtdataType != DataPartition)
		return VSXE_BAD_EXTDATA_TYPE;
	if(ext->Files.Count != 1)
		return VSXE_UNEXPECTED_DATA_IN_EXTDATA;
	if(ext->VSXE_Extdata_ID_Match != True){
		printf("[!] Caution, extdata image '%s' did not have expected identifier\n",inpath);
	}
	WriteBuffer((ext->extdata.buffer + ext->Files.Data[0].offset),ext->Files.Data[0].size,0,out);
	FreeExtdataContext(ext);
	return 0;
}

void VSXE_ReturnDirPath(u32 folder_id, char *path, u8 platform)
{
	// Counting Number of directories to root directory
	u32 path_part_count = 0;
	u32 present_dir = u8_to_u32(vsxe_ctx.folders[folder_id].parent_folder_index,LE);
	while(present_dir > 1){
		path_part_count++;
		present_dir = u8_to_u32(vsxe_ctx.folders[present_dir].parent_folder_index,LE);
	}
	
	// Storing the folder entry indexes to root
	u32 folderlocation[path_part_count];
	present_dir = u8_to_u32(vsxe_ctx.folders[folder_id].parent_folder_index,LE);
	for(u32 i = path_part_count; present_dir > 0; i--){
		if(i!=path_part_count)
			present_dir = u8_to_u32(vsxe_ctx.folders[present_dir].parent_folder_index,LE);
		folderlocation[i] = present_dir;
		if(!i)
			break;
	}
	
	sprintf(path,"%s%c",path,platform);
	
	// Adding each directory to the path
	for(u32 i = 1; i < path_part_count; i++){
		u8 folder_id = folderlocation[i];	
		sprintf(path,"%s%s%c",path,vsxe_ctx.folders[folder_id].filename,platform);
	}
	sprintf(path,"%s%s",path,vsxe_ctx.folders[folder_id].filename);	
}

void VSXE_ReturnExtdataMountPath(u32 file_id, char *path, u8 platform)
{
	// Counting Number of directories to root directory
	u32 path_part_count = 0;
	u32 present_dir = u8_to_u32(vsxe_ctx.files[file_id].parent_folder_index,LE);
	while(present_dir > 1){
		path_part_count++;
		present_dir = u8_to_u32(vsxe_ctx.folders[present_dir].parent_folder_index,LE);
	}
	
	// Storing the folder entry indexes to root
	u32 folderlocation[path_part_count];
	present_dir = u8_to_u32(vsxe_ctx.files[file_id].parent_folder_index,LE);
	for(u32 i = path_part_count; present_dir > 0; i--){
		if(i!=path_part_count)
			present_dir = u8_to_u32(vsxe_ctx.folders[present_dir].parent_folder_index,LE);
		folderlocation[i] = present_dir;
		if(!i)
			break;
	}	
	
	// Adding each directory to the path
	for(u32 i = 1; i <= path_part_count; i++){
		u8 folder_id = folderlocation[i];
		sprintf(path,"%s%s%c",path,vsxe_ctx.folders[folder_id].filename,platform);
	}
	sprintf(path,"%s%s",path,vsxe_ctx.files[file_id].filename);	
}
